cmake_minimum_required (VERSION 2.6)
set(CMAKE_C_COMPILER                  "/usr/bin/clang-3.8")
set(CMAKE_CXX_COMPILER                "/usr/bin/clang++-3.8")

project (TradeEngine)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS                   "-g -O0 -Wall -std=c++11 -v -lSegFault")
set(BASEPATH                          "${CMAKE_SOURCE_DIR}/src")
include_directories(${BASEPATH})

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
        src/Thread/TaskQueueThread.h
        src/Common.h

        src/Net/UDPSocket.h
        src/Net/APIDataPackage.h
)

set(
    SOURCES
        src/Common.cpp
        src/Thread/Threader.cpp
        src/Thread/ThreadManager.cpp
        src/Thread/SocketPollThread.cpp
        src/Thread/TaskQueueThread.cpp
)

add_executable(trader ${HEADERS} ${SOURCES} src/main.cpp)
target_link_libraries(trader pthread)
project (TradeEngine_test)
add_subdirectory(${CMAKE_SOURCE_DIR}/gtest)

enable_testing()
set(PROJECT_TEST_NAME TradeEngine_test)
include_directories(${GTEST_INCLUDE_DIRS} ${BASEPATH})
file(GLOB TEST_SRC_FILES ${BASEPATH}/test/*.cpp)
add_executable(${PROJECT_TEST_NAME} ${HEADERS} ${SOURCES} ${TEST_SRC_FILES})
add_dependencies(${PROJECT_TEST_NAME} googletest)

target_link_libraries(${PROJECT_TEST_NAME}
    ${GTEST_LIBS_DIR}/libgtest.a
    ${GTEST_LIBS_DIR}/libgtest_main.a
)

target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_compile_definitions(${PROJECT_TEST_NAME} PUBLIC -DIS_TEST)
add_test(test1 ${PROJECT_TEST_NAME})
