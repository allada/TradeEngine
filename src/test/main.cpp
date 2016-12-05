#include "gtest/gtest.h"

#include "Thread/MainThread.h"
#include "Thread/ThreadManager.h"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::shared_ptr<Thread::MainThread> mainThread;
    mainThread = std::make_shared<Thread::MainThread>();
    Thread::ThreadManager::setMainThread(mainThread);

    return RUN_ALL_TESTS();
}