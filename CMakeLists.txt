cmake_minimum_required (VERSION 2.6)
project (TradeEngine)
set(CMAKE_C_COMPILER                  "/usr/bin/clang")
set(CMAKE_CXX_COMPILER                "/usr/bin/clang++-3.8")

set(CMAKE_CXX_FLAGS                   "-g -O0 -Wall -std=c++11 -v -lSegFault")
set(BASEPATH                          "${CMAKE_SOURCE_DIR}/src")
include_directories("${BASEPATH}")

set(
    HEADERS
        src/base/Thread/QueueThread.h
        src/base/Thread/Thread.h
        src/base/Thread/Threader.h
        src/base/Thread/MainThread.h
        src/base/Thread/ThreadEventer.h
        src/base/Thread/CrossThreadEventer.h
        src/base/Thread/ThreadManager.h
        src/base/Thread/ThreadQueue.h
        src/base/Common.h

        #src/net/SocketPoolTaskRunner.h
        src/net/sockets/DataGramSocket.h
        src/net/sockets/EventSocket.h
        src/net/sockets/UDPSocket.h
        src/net/sockets/SocketEventDeligate.h
)

set(
    SOURCES
        src/base/Thread/ThreadManager.cpp
        src/base/Thread/CrossThreadEventer.cpp
        src/base/Thread/QueueThread.cpp
        #src/net/SocketPoolTaskRunner.cpp
        src/net/sockets/DataGramSocket.cpp
        src/net/sockets/UDPSocket.cpp
        src/main.cpp
)

add_executable(trader ${HEADERS} ${SOURCES})
target_link_libraries(trader pthread)
