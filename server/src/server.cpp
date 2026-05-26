#include "server.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <ctime>
#include <algorithm>

ChatServer::ChatServer(int port) : port_(port) {
    db_ = std::make_unique<ChatDB>("chat.db");
    initListen();
}

ChatServer::~ChatServer() {
    if (listen_fd_ >= 0) close(listen_fd_);
    if (epoll_fd_ >= 0) close(epoll_fd_);
}

void ChatServer::initListen() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listen_fd_ < 0) throw std::runtime_error("socket");

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET; addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd_, SOMAXCONN);

    epoll_fd_ = epoll_create1(0);
    struct epoll_event ev{}; ev.events = EPOLLIN; ev.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);

    std::cout << "[server] listening on ::" << port_ << " (SQLite)" << std::endl;
}

void ChatServer::start() {
    running_ = true;
    struct epoll_event events[64];
    while (running_) {
        int n = epoll_wait(epoll_fd_, events, 64, -1);
        if (n < 0) { if (errno == EINTR) continue; break; }
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == listen_fd_) handleAccept();
            else if (events[i].events & (EPOLLERR | EPOLLHUP)) closeClient(fd);
            else if (events[i].events & EPOLLIN) handleRead(fd);
        }
    }
}

void ChatServer::handleAccept() {
    while (true) {
        int fd = accept4(listen_fd_, nullptr, nullptr, SOCK_NONBLOCK);
        if (fd < 0) break;
        int one = 1; setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        struct epoll_event ev{}; ev.events = EPOLLIN | EPOLLET; ev.data.fd = fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
        clients_[fd] = {fd, "", "user"};
    }
}

void ChatServer::handleRead(int fd) {
    auto it = clients_.find(fd);
    if (it == clients_.end()) return;

    char buf[65536];
    static thread_local std::unordered_map<int, std::string> rbufs;
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) { closeClient(fd); return; }

    std::string& rbuf = rbufs[fd];
    rbuf.append(buf, n);

    while (rbuf.size() >= 4) {
        uint32_t len; memcpy(&len, rbuf.data(), 4);
        if (rbuf.size() < 4 + len) break;
        std::string msg = rbuf.substr(4, len);
        rbuf.erase(0, 4 + len);

        auto f = [&](const std::string& key) -> std::string {
            auto pos = msg.find("\"" + key + "\":\"");
            if (pos == std::string::npos) return "";
            pos += key.size() + 4;
            auto end = msg.find('"', pos);
            return (end != std::string::npos) ? msg.substr(pos, end - pos) : "";
        };

        std::string type = f("type");
        if (type == "register")      onRegister(fd, f("username"), f("password"));
        else if (type == "login")    onLogin(fd, f("username"), f("password"));
        else if (type == "msg")      onMessage(fd, f("to"), f("content"));
        else if (type == "list")     onList(fd);
        else if (type == "history")  onHistory(fd, f("with"));
        else if (type == "add_contact") onAddContact(fd, f("contact"));
        else if (type == "get_contacts") onGetContacts(fd);
        else if (type == "admin_users") onAdminUsers(fd);
    }
}

// ========== 消息处理 ==========

void ChatServer::onRegister(int fd, const std::string& user, const std::string& pass) {
    if (user.empty() || pass.empty()) {
        sendJson(fd, "{\"type\":\"error\",\"msg\":\"用户名或密码不能为空\"}");
        return;
    }
    if (db_->registerUser(user, pass)) {
        db_->addContact(user, user);  // 自己是自己的联系人
        sendJson(fd, "{\"type\":\"register_ok\"}");
        std::cout << "[server] " << user << " registered" << std::endl;
    } else {
        sendJson(fd, "{\"type\":\"error\",\"msg\":\"用户名已存在\"}");
    }
}

void ChatServer::onLogin(int fd, const std::string& user, const std::string& pass) {
    if (!db_->verifyPassword(user, pass)) {
        sendJson(fd, "{\"type\":\"error\",\"msg\":\"用户名或密码错误\"}");
        return;
    }

    // 踢旧连接
    auto old = users_.find(user);
    if (old != users_.end()) { sendJson(old->second, "{\"type\":\"kicked\"}"); closeClient(old->second); }

    clients_[fd].username = user;
    clients_[fd].role = db_->isAdmin(user) ? "admin" : "user";
    users_[user] = fd;

    db_->updateLastLogin(user);

    std::ostringstream oss;
    oss << "{\"type\":\"login_ok\",\"username\":\"" << user
        << "\",\"role\":\"" << clients_[fd].role << "\"}";
    sendJson(fd, oss.str());

    // 发送联系人列表
    onGetContacts(fd);

    std::cout << "[server] " << user << " (" << clients_[fd].role << ") logged in" << std::endl;
}

void ChatServer::onMessage(int fd, const std::string& to, const std::string& content) {
    auto& cli = clients_[fd];
    if (cli.username.empty() || to.empty() || content.empty()) return;

    char tbuf[64]; time_t now = time(nullptr);
    strftime(tbuf, sizeof(tbuf), "%H:%M", localtime(&now));

    // 持久化
    db_->saveMessage(cli.username, to, content, tbuf);

    std::ostringstream oss;
    oss << "{\"type\":\"msg\",\"from\":\"" << cli.username
        << "\",\"to\":\"" << to << "\",\"content\":\"" << content
        << "\",\"time\":\"" << tbuf << "\"}";
    std::string msg = oss.str();

    // 发给接收方
    auto ti = users_.find(to);
    if (ti != users_.end()) sendJson(ti->second, msg);

    // 回执
    sendJson(fd, msg);
}

void ChatServer::onList(int fd) {
    std::ostringstream oss;
    oss << "{\"type\":\"user_list\",\"users\":[";
    bool first = true;
    for (auto& [_, cli] : clients_) {
        if (cli.username.empty()) continue;
        if (!first) oss << ",";
        oss << "{\"name\":\"" << cli.username << "\",\"role\":\"" << cli.role << "\"}";
        first = false;
    }
    oss << "]}";
    sendJson(fd, oss.str());
}

void ChatServer::onHistory(int fd, const std::string& with) {
    auto& cli = clients_[fd];
    if (cli.username.empty() || with.empty()) return;

    auto msgs = db_->getHistory(cli.username, with, 50);
    std::ostringstream oss;
    oss << "{\"type\":\"history\",\"with\":\"" << with << "\",\"messages\":[";
    // 反转（数据库是倒序的）
    for (int i = msgs.size() - 1; i >= 0; --i) {
        if (i < (int)msgs.size() - 1) oss << ",";
        oss << "{\"from\":\"" << msgs[i].from
            << "\",\"to\":\"" << msgs[i].to
            << "\",\"content\":\"" << msgs[i].content
            << "\",\"time\":\"" << msgs[i].time << "\"}";
    }
    oss << "]}";
    sendJson(fd, oss.str());
}

void ChatServer::onAddContact(int fd, const std::string& contact) {
    auto& cli = clients_[fd];
    if (cli.username.empty() || contact.empty()) return;

    if (db_->addContact(cli.username, contact)) {
        sendJson(fd, "{\"type\":\"contact_added\",\"contact\":\"" + contact + "\"}");
        // 通知对方也加为联系人
        db_->addContact(contact, cli.username);
    } else {
        sendJson(fd, "{\"type\":\"error\",\"msg\":\"添加失败\"}");
    }
}

void ChatServer::onGetContacts(int fd) {
    auto& cli = clients_[fd];
    auto contacts = db_->getContacts(cli.username);

    std::ostringstream oss;
    oss << "{\"type\":\"contacts\",\"contacts\":[";
    for (size_t i = 0; i < contacts.size(); ++i) {
        if (i) oss << ",";
        oss << "\"" << contacts[i] << "\"";
    }
    oss << "]}";
    sendJson(fd, oss.str());
}

void ChatServer::onAdminUsers(int fd) {
    auto& cli = clients_[fd];
    if (cli.role != "admin") {
        sendJson(fd, "{\"type\":\"error\",\"msg\":\"需要管理员权限\"}");
        return;
    }
    auto users = db_->getAllUsers();
    std::ostringstream oss;
    oss << "{\"type\":\"all_users\",\"users\":[";
    for (size_t i = 0; i < users.size(); ++i) {
        if (i) oss << ",";
        oss << "{\"name\":\"" << users[i].username
            << "\",\"role\":\"" << users[i].role
            << "\",\"last\":\"" << users[i].lastLogin << "\"}";
    }
    oss << "]}";
    sendJson(fd, oss.str());
}

// ========== 工具 ==========

void ChatServer::sendJson(int fd, const std::string& json) {
    uint32_t len = json.size();
    char header[4]; memcpy(header, &len, 4);
    std::string packet(header, 4); packet += json;
    send(fd, packet.data(), packet.size(), MSG_NOSIGNAL);
}

void ChatServer::closeClient(int fd) {
    auto it = clients_.find(fd);
    if (it != clients_.end()) {
        if (!it->second.username.empty()) {
            users_.erase(it->second.username);
            std::cout << "[server] " << it->second.username << " disconnected" << std::endl;
        }
        clients_.erase(it);
    }
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}
