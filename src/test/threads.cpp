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

TEST(ThreadTest, SocketPollThreadKills) {
    {
        std::shared_ptr<SocketPollThread> thread = createThread<SocketPollThread>("Thread");
        thread->kill();
        thread->join();
    }
    EXPECT_EQ(ThreadManager::activeThreadsCount(), 1);
}

TEST(ThreadTest, QueueVerifyData) {
    class Producer : public Tasker {
    public:
        Producer(std::mutex& mux, std::condition_variable& cv, TaskQueue<uint8_t, 12>& dataQueue, bool& running)
            : lock(mux)
            , cv(&cv)
            , dataQueue(&dataQueue)
            , running(&running)
        {
        }

        void run() override {
            *running = true;
            {
                dataQueue->push('1');
            }
            cv->notify_one();
            cv->wait(lock);
            {
                std::array<uint8_t, 12> list = {{'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C'}};
                dataQueue->pushChunk(list);
            }
            cv->notify_one();
            cv->wait(lock);
            {
                std::array<uint8_t, 6> list = {{'F', '0', 'O', 'B', 'A', 'R'}};
                dataQueue->pushChunk(list);
            }
            cv->notify_one();
            cv->wait(lock);
            {
                std::array<uint8_t, 12> list = {{'T', 'H', 'I', 'S', 'I', 'S', 'M', 'U', 'C', 'H', 'M', 'O'}};
                dataQueue->pushChunk(list);
            }
            cv->notify_one();
        }

    private:
        std::unique_lock<std::mutex> lock;
        std::condition_variable* cv;
        TaskQueue<uint8_t, 12>* dataQueue;
        bool* running;

    };

    TaskQueue<uint8_t, 12> dataQueue;
    std::mutex mux;
    std::unique_lock<std::mutex> lock(mux, std::defer_lock);
    std::condition_variable cv;
    bool running = false;

    std::shared_ptr<TaskQueueThread> thread = createThread<TaskQueueThread>("Producer");
    thread->addTask(WrapUnique(new Producer(mux, cv, dataQueue, running)));
    lock.lock();
    cv.wait(lock, [running]{ return running; });

    std::vector<uint8_t> list(1);
    dataQueue.popChunk(list);
    EXPECT_EQ(list[0], '1');

    cv.notify_one();
    cv.wait(lock);

    list.resize(12);
    dataQueue.popChunk(list);
    EXPECT_EQ(list[0], '1');
    EXPECT_EQ(list[1], '2');
    EXPECT_EQ(list[2], '3');
    EXPECT_EQ(list[3], '4');
    EXPECT_EQ(list[4], '5');
    EXPECT_EQ(list[5], '6');
    EXPECT_EQ(list[6], '7');
    EXPECT_EQ(list[7], '8');
    EXPECT_EQ(list[8], '9');
    EXPECT_EQ(list[9], 'A');
    EXPECT_EQ(list[10], 'B');
    EXPECT_EQ(list[11], 'C');

    cv.notify_one();
    cv.wait(lock);

    list.resize(6);
    dataQueue.popChunk(list);
    EXPECT_EQ(list[0], 'F');
    EXPECT_EQ(list[1], '0');
    EXPECT_EQ(list[2], 'O');
    EXPECT_EQ(list[3], 'B');
    EXPECT_EQ(list[4], 'A');
    EXPECT_EQ(list[5], 'R');

    cv.notify_one();
    cv.wait(lock);

    list.resize(9);
    dataQueue.popChunk(list);
    EXPECT_EQ(list[0], 'T');
    EXPECT_EQ(list[1], 'H');
    EXPECT_EQ(list[2], 'I');
    EXPECT_EQ(list[3], 'S');
    EXPECT_EQ(list[4], 'I');
    EXPECT_EQ(list[5], 'S');
    EXPECT_EQ(list[6], 'M');
    EXPECT_EQ(list[7], 'U');
    EXPECT_EQ(list[8], 'C');

    list.resize(3);
    dataQueue.popChunk(list);
    EXPECT_EQ(list[0], 'H');
    EXPECT_EQ(list[1], 'M');
    EXPECT_EQ(list[2], 'O');

    lock.unlock();
    thread->kill();
    thread->join();
}

} // namespace
