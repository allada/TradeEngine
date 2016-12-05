#include "gtest/gtest.h"

#include "Thread/SocketPollThread.h"
#include "Thread/TaskQueueThread.h"
#include "Thread/ThreadManager.h"

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
