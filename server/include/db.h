#pragma once

#include <string>
#include <vector>
#include <functional>

/**
 * ChatDB — SQLite 聊天数据库
 *
 * 表结构：
 *   users(id, username, password_hash, role, last_login)
 *   messages(id, from_user, to_user, content, time)
 *   contacts(username, contact_username) UNIQUE
 */
class ChatDB {
public:
    struct UserRow {
        std::string username;
        std::string role;       // "admin" | "user"
        std::string lastLogin;
    };

    struct MsgRow {
        std::string from;
        std::string to;
        std::string content;
        std::string time;
    };

    explicit ChatDB(const std::string& path = "chat.db");
    ~ChatDB();

    // 用户
    bool registerUser(const std::string& username, const std::string& password);
    bool verifyPassword(const std::string& username, const std::string& password);
    UserRow getUser(const std::string& username);
    void updateLastLogin(const std::string& username);
    bool isAdmin(const std::string& username);

    // 消息
    void saveMessage(const std::string& from, const std::string& to,
                     const std::string& content, const std::string& time);
    std::vector<MsgRow> getHistory(const std::string& user1,
                                    const std::string& user2,
                                    int limit = 50);

    // 联系人
    bool addContact(const std::string& user, const std::string& contact);
    std::vector<std::string> getContacts(const std::string& user);

    // 管理员
    std::vector<UserRow> getAllUsers();

private:
    void initTables();
    std::string hashPassword(const std::string& password);

    void* db_;  // sqlite3*
};
