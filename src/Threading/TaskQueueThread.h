#ifndef TaskQueueThread_h
#define TaskQueueThread_h

#include "Thread.h"
#include "TaskQueue.h"
#include "Tasker.h"
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace Threading {

#define MAX_QUEUE_SIZE 10485750

class TaskQueueThread : public Thread {
public:
    class TaskQueueSignal : public Tasker {
    public:
        TaskQueueSignal(Signals signal)
            : signal_(signal) { }

        void run() override
        {
            switch (signal_) {
                case Signals::Kill:
                    DEBUG("Got Kill signal");
                    thisThread()->kill();
                    break;
            }
        }

    private:
        Signals signal_;
    };

    TaskQueueThread(std::unique_ptr<std::thread> thread, const std::string& name)
        : Thread(std::move(thread), name)
        , running_(true)
        , processing_items_(MAX_QUEUE_SIZE) { }

    void addTask(std::unique_ptr<Tasker> task)
    {
        EXPECT_NE(task.get(), nullptr);
        taskQueue_.push(std::move(task));
        //if (!sent_notification_.test_and_set()) {
            notifier_cv_.notify_one();
        //}
    }
    void kill() override
    {
        if (id() == thisThreadId()) {
            DEBUG("Got shutdown command");
            running_ = false;
        } else {
            DEBUG("Killing thread: %s", this->name().c_str());
            sendSignal(Signals::Kill);
        }
    }

    void entryPoint() override
    {
        std::unique_lock<std::mutex> lock(notifier_mux_);
        while (running_) {
            auto len = taskQueue_.countAsReader();
            processing_items_.clear();
            if (!len) {
                // Wake every 100 miliseconds and try to ensure we did not leave any stragglers.
                sent_notification_.clear();
                notifier_cv_.wait_for(lock, std::chrono::milliseconds(100));
                continue;
            }
            taskQueue_.popChunk(processing_items_, len);
            for (auto i = 0; i < len; i++) {
                EXPECT_NE(processing_items_[i].get(), nullptr);
                processing_items_[i]->run();
            }
        }
        DEBUG("Exited thread loop");
    }

    void sendSignal(Signals signal) override
    {
        addTask(WrapUnique(new TaskQueueSignal(signal)));
    }

private:
    bool running_;

    std::mutex notifier_mux_;
    std::condition_variable notifier_cv_;
    std::atomic_flag sent_notification_ = ATOMIC_FLAG_INIT;
    TaskQueue<std::unique_ptr<Tasker>, MAX_QUEUE_SIZE> taskQueue_;

    // This is down here because it's faster to allocate the item in the heap once.
    std::vector<std::unique_ptr<Tasker>> processing_items_;

};

} /* Thread */

#endif /* TaskQueueThread_h */
