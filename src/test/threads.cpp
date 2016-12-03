#include "gtest/gtest.h"

#include <csignal>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include "Thread/SocketPollThread.h"
#include "Thread/TaskQueueThread.h"
#include "Thread/Threader.h"
#include "Thread/ThreadManager.h"
#include "Thread/MainThread.h"

#include <chrono>
#include <unistd.h>

namespace {
using namespace Thread;

class ThreadTest : public ::testing::Test {
};


TEST(ThreadTest, TaskQueueThreadKills) {
    {
        std::shared_ptr<TaskQueueThread> thread = createThread<TaskQueueThread>("Thread");
        thread->kill();
        thread->join();
    }
    EXPECT_EQ(ThreadManager::activeThreadsCount(), 1);
}

TEST(ThreadTest, SocketPoolThreadKills) {
    {
        std::shared_ptr<SocketPollThread> thread = createThread<SocketPollThread>("Thread");
        thread->kill();
        thread->join();
    }
    EXPECT_EQ(ThreadManager::activeThreadsCount(), 1);
}

} // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::shared_ptr<MainThread> mainThread;
    mainThread = std::make_shared<MainThread>();
    ThreadManager::setMainThread(mainThread);

    return RUN_ALL_TESTS();
}