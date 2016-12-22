#include "TaskQueueThread.h"
#include <chrono>

using namespace Threading;

void TaskQueueThread::kill()
{
    if (id() == thisThreadId()) {
        DEBUG("Got shutdown command");
        running_ = false;
    } else {
        DEBUG("Killing thread: %s", this->name().c_str());
        sendSignal(Signals::Kill);
    }
}

void TaskQueueThread::addTask(std::unique_ptr<Tasker> task)
{
    EXPECT_NE(task.get(), nullptr);
    taskQueue_.push(std::move(task));
}

void TaskQueueThread::entryPoint()
{
    //std::unique_lock<std::mutex> lock(notifier_mux_);
    while (running_) {
        // Wake every 50 miliseconds and try to ensure we did not leave any stragglers.
        //notifier_cv_.wait_for(lock, std::chrono::milliseconds(1));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto len = taskQueue_.countAsReader();
        std::vector<std::unique_ptr<Tasker>> items(len);
        taskQueue_.popChunk(items, len);
        for (auto& task : items) {
            EXPECT_NE(task.get(), nullptr);
            task->run();
        }
    }
    DEBUG("Exited thread loop");
}

void TaskQueueThread::sendSignal(Signals signal)
{
    addTask(WrapUnique(new TaskQueueSignal(signal)));
}
