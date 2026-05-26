#include "server.h"
#include <iostream>
#include <csignal>

static ChatServer* g_server = nullptr;

static void sigHandler(int) {
    std::cout << "\n[chat-server] shutting down..." << std::endl;
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    int port = (argc > 1) ? std::stoi(argv[1]) : 9000;

    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    try {
        ChatServer server(port);
        g_server = &server;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
