#include "TaskQueueThread.h"
#include <chrono>
//#include "../Common.h"

using namespace Thread;

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
    taskQueue_.push(std::move(task));
    notifier_cv_.notify_one();
}

void TaskQueueThread::entryPoint()
{
    std::unique_lock<std::mutex> lock(notifier_mux_);
    while (running_) {
        // Wake every 50 miliseconds and try to ensure we did not leave any stragglers.
        //notifier_cv_.wait_for(notifier_mux_, std::chrono::milliseconds(50));
        auto len = taskQueue_.countAsReader();
        for (auto i = len; i > 0 && running_; --i) {
            std::unique_ptr<Tasker> task = taskQueue_.pop();
            task->run();
        }
    }
    DEBUG("Exited thread loop");
}

void TaskQueueThread::sendSignal(Signals signal)
{
    addTask(WrapUnique(new TaskQueueSignal(signal)));
}
