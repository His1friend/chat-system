#include "db.h"
#include "sqlite3.h"
#include "sha256.h"
#include <cstring>
#include <iostream>

ChatDB::ChatDB(const std::string& path) {
    if (sqlite3_open(path.c_str(), (sqlite3**)&db_) != SQLITE_OK) {
        std::cerr << "[db] open failed: " << sqlite3_errmsg((sqlite3*)db_) << std::endl;
        db_ = nullptr;
        return;
    }
    sqlite3_exec((sqlite3*)db_, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
    initTables();
}

ChatDB::~ChatDB() {
    if (db_) sqlite3_close((sqlite3*)db_);
}

void ChatDB::initTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT DEFAULT 'user',
            last_login TEXT DEFAULT ''
        );
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_user TEXT NOT NULL,
            to_user TEXT NOT NULL,
            content TEXT NOT NULL,
            time TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS contacts (
            username TEXT NOT NULL,
            contact_username TEXT NOT NULL,
            UNIQUE(username, contact_username)
        );
        -- 默认管理员 admin/admin
        INSERT OR IGNORE INTO users(username, password_hash, role)
        VALUES('admin', '" + sha256("admin") + R"(', 'admin');
    )";
    char* err = nullptr;
    sqlite3_exec((sqlite3*)db_, sql, nullptr, nullptr, &err);
    if (err) { std::cerr << "[db] init error: " << err << std::endl; sqlite3_free(err); }
}

bool ChatDB::registerUser(const std::string& username, const std::string& password) {
    std::string hash = sha256(password);
    std::string sql = "INSERT INTO users(username, password_hash) VALUES(?,?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool ChatDB::verifyPassword(const std::string& username, const std::string& password) {
    std::string hash = sha256(password);
    std::string sql = "SELECT password_hash FROM users WHERE username=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* stored = (const char*)sqlite3_column_text(stmt, 0);
        ok = stored && hash == stored;
    }
    sqlite3_finalize(stmt);
    return ok;
}

ChatDB::UserRow ChatDB::getUser(const std::string& username) {
    UserRow row;
    std::string sql = "SELECT username, role, last_login FROM users WHERE username=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        row.username  = (const char*)sqlite3_column_text(stmt, 0);
        row.role      = (const char*)sqlite3_column_text(stmt, 1) ?: "user";
        row.lastLogin = (const char*)sqlite3_column_text(stmt, 2) ?: "";
    }
    sqlite3_finalize(stmt);
    return row;
}

void ChatDB::updateLastLogin(const std::string& username) {
    std::string sql = "UPDATE users SET last_login=datetime('now') WHERE username=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool ChatDB::isAdmin(const std::string& username) {
    auto u = getUser(username);
    return u.role == "admin";
}

void ChatDB::saveMessage(const std::string& from, const std::string& to,
                          const std::string& content, const std::string& time) {
    std::string sql = "INSERT INTO messages(from_user, to_user, content, time) VALUES(?,?,?,?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, from.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, to.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, time.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<ChatDB::MsgRow> ChatDB::getHistory(const std::string& user1,
                                                 const std::string& user2,
                                                 int limit) {
    std::vector<MsgRow> result;
    std::string sql = R"(
        SELECT from_user, to_user, content, time FROM messages
        WHERE (from_user=? AND to_user=?) OR (from_user=? AND to_user=?)
        ORDER BY id DESC LIMIT ?
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, user1.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user2.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user2.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user1.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MsgRow m;
        m.from    = (const char*)sqlite3_column_text(stmt, 0);
        m.to      = (const char*)sqlite3_column_text(stmt, 1);
        m.content = (const char*)sqlite3_column_text(stmt, 2);
        m.time    = (const char*)sqlite3_column_text(stmt, 3);
        result.push_back(m);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool ChatDB::addContact(const std::string& user, const std::string& contact) {
    std::string sql = "INSERT OR IGNORE INTO contacts(username, contact_username) VALUES(?,?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, contact.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<std::string> ChatDB::getContacts(const std::string& user) {
    std::vector<std::string> result;
    std::string sql = "SELECT contact_username FROM contacts WHERE username=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back((const char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<ChatDB::UserRow> ChatDB::getAllUsers() {
    std::vector<UserRow> result;
    std::string sql = "SELECT username, role, last_login FROM users";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db_, sql.c_str(), -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserRow r;
        r.username  = (const char*)sqlite3_column_text(stmt, 0);
        r.role      = (const char*)sqlite3_column_text(stmt, 1) ?: "user";
        r.lastLogin = (const char*)sqlite3_column_text(stmt, 2) ?: "";
        result.push_back(r);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::string ChatDB::hashPassword(const std::string& password) {
    return sha256(password);
}
