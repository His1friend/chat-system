cmake_minimum_required(VERSION 3.16)
project(chat-server VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g -Wno-unused-function")

find_package(Threads REQUIRED)

add_executable(chat-server
    src/main.cpp
    src/server.cpp
    src/db.cpp
    src/sha256.cpp
    src/sqlite3.c
)

target_include_directories(chat-server PRIVATE include)
target_link_libraries(chat-server PRIVATE Threads::Threads dl)
