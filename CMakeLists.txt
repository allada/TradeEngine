cmake_minimum_required (VERSION 2.6)
project (TradeEngine)
set(CMAKE_C_COMPILER                  "/usr/bin/clang")
set(CMAKE_CXX_COMPILER                "/usr/bin/clang++-3.8")

set(CMAKE_CXX_FLAGS                   "-g -O0 -Wall -std=c++11 -v -lSegFault")
set(BASEPATH                          "${CMAKE_SOURCE_DIR}/src")
include_directories("${BASEPATH}")

set(
    HEADERS
        src/Thread/SocketPollThread.h
        src/Thread/SocketTasker.h
        src/Thread/Thread.h
        src/Thread/Threader.h
        src/Thread/Tasker.h
        src/Thread/MainThread.h
        src/Thread/ThreadManager.h
        src/Thread/TaskQueue.h
        src/Common.h
)

set(
    SOURCES
        src/Common.cpp
        src/Thread/Threader.cpp
        src/Thread/ThreadManager.cpp
        src/Thread/SocketPollThread.cpp
        src/main.cpp
)

add_executable(trader ${HEADERS} ${SOURCES})
target_link_libraries(trader pthread)
