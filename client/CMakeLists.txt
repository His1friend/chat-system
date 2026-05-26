cmake_minimum_required(VERSION 3.16)
project(wechat-client VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_executable(wechat-client
    src/main.cpp
    src/networkclient.cpp
    src/loginwindow.cpp
    src/mainwindow.cpp
    src/chatlist.cpp
    src/chatview.cpp
)

target_link_libraries(wechat-client PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
