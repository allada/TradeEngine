cmake_minimum_required (VERSION 2.6)
set(CMAKE_C_COMPILER                  "/usr/bin/clang-3.8")
set(CMAKE_CXX_COMPILER                "/usr/bin/clang++-3.8")

project (TradeEngine)

if("$ENV{DEBUG_OUTPUT}" STREQUAL "1")
    message(STATUS "Running with debug output on")
    set(DEBUG_OUTPUT "-DDEBUG_OUTPUT")
else()
    message(STATUS "Running with debug output off")
    set(DEBUG_OUTPUT "")
endif()

find_package(Threads REQUIRED)

if("$ENV{OPRIMIZED}" STREQUAL "1")
    set(CMAKE_CXX_FLAGS                   "-Ofast -march=native -Wall -std=c++11 ${DEBUG_OUTPUT}")
else()
    set(CMAKE_CXX_FLAGS                   "-g -O0 -Wall -std=c++11 ${DEBUG_OUTPUT}")
endif()

set(BASEPATH                          "${CMAKE_SOURCE_DIR}/src")
include_directories(${BASEPATH})

set(
    HEADERS
        src/Threading/SocketPollThread.h
        src/Threading/SocketTasker.h
        src/Threading/Thread.h
        src/Threading/Threader.h
        src/Threading/Tasker.h
        src/Threading/MainThread.h
        src/Threading/ThreadManager.h
        src/Threading/TaskQueue.h
        src/Threading/TaskQueueThread.h
        src/Common.h

        src/includes/crc32.h
        src/includes/MemoryAllocator.h

        src/API/DataPackage.h
        src/API/StreamDispatcher.h

        src/Net/UDPSocketRecvTask.h

        src/Engine/BuyLedger.h
        src/Engine/Order.h
        src/Engine/SellLedger.h
        src/Engine/Trade.h
        src/Engine/ProcessOrderTask.h
        src/Engine/Ledger.h
)

set(
    SOURCES
        src/Common.cpp
        src/Threading/Thread.cpp
        src/Threading/Threader.cpp
        src/Threading/ThreadManager.cpp
        src/Threading/SocketPollThread.cpp

        src/includes/MemoryAllocator.cpp

        src/Net/UDPSocketRecvTask.cpp

        src/Engine/SellLedger.cpp
        src/Engine/BuyLedger.cpp
        src/Engine/Trade.cpp
)

add_executable(trader ${HEADERS} ${SOURCES} src/main.cpp)
target_link_libraries(trader pthread Judy)

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
target_link_libraries(${PROJECT_TEST_NAME} pthread Judy)
