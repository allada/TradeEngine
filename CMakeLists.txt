cmake_minimum_required (VERSION 2.6)
project (TradeEngine)
set(CMAKE_CXX_FLAGS                   "-g -O0 -Wall -std=c++11 -v -lSegFault")
set(BASEPATH                          "${CMAKE_SOURCE_DIR}/src")
include_directories("${BASEPATH}")

set(
    HEADERS
        src/net/SocketPoolTaskRunner.h
        src/net/sockets/DataGramSocket.h
        src/net/sockets/EventSocket.h
        src/net/sockets/UDPSocket.h
        src/net/sockets/SocketEventDeligate.h
)

set(
    SOURCES
        src/net/SocketPoolTaskRunner.cpp
        src/net/sockets/DataGramSocket.cpp
        src/net/sockets/UDPSocket.cpp
        src/main.cpp
)

add_executable(trader ${HEADERS} ${SOURCES})
target_link_libraries(trader pthread)
