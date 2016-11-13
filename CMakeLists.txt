cmake_minimum_required (VERSION 2.6)
project (TradeEngine)
set(CMAKE_CXX_FLAGS                   "-g -Wall -std=c++11 -v")
set(BASEPATH                          "${CMAKE_SOURCE_DIR}/src")
include_directories("${BASEPATH}")

set(
    SRC_LIST
        src/main.cpp
        src/net/SocketPoolTaskRunner.cpp
        src/net/sockets/DataGramSocket.cpp
        src/net/sockets/EventSocket.cpp
        src/net/sockets/UDPSocket.cpp
)

add_executable(trader ${SRC_LIST})
