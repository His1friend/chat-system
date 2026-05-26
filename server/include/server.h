#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include "db.h"

class ChatServer {
public:
    struct Client {
        int fd;
        std::string username;
        std::string role;       // "admin" | "user"
    };

    ChatServer(int port = 9000);
    ~ChatServer();

    void start();
    void stop() { running_ = false; }

private:
    void initListen();
    void handleAccept();
    void handleRead(int fd);
    void closeClient(int fd);

    // 消息处理
    void onRegister(int fd, const std::string& user, const std::string& pass);
    void onLogin(int fd, const std::string& user, const std::string& pass);
    void onMessage(int fd, const std::string& to, const std::string& content);
    void onList(int fd);
    void onHistory(int fd, const std::string& with);
    void onAddContact(int fd, const std::string& contact);
    void onGetContacts(int fd);
    void onAdminUsers(int fd);

    void sendJson(int fd, const std::string& json);

    int port_;
    int listen_fd_ = -1;
    int epoll_fd_  = -1;
    bool running_  = false;

    std::unique_ptr<ChatDB> db_;

    // fd → Client
    std::unordered_map<int, Client> clients_;
    // username → fd
    std::unordered_map<std::string, int> users_;
};
