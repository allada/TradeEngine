#ifndef TaskQueueThread_h
#define TaskQueueThread_h

#include "Thread.h"
#include "TaskQueue.h"
#include "Tasker.h"
#include <condition_variable>
#include <mutex>

namespace Thread {

class TaskQueueThread : public Thread<TaskQueueThread> {
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
        , running_(true) { }

    void addTask(std::unique_ptr<Tasker>);
    void kill() override;
    void entryPoint() override;
    void sendSignal(Signals) override;

private:
    bool running_;
    std::mutex notifier_mux_;
    std::condition_variable notifier_cv_;
    TaskQueue<std::unique_ptr<Tasker>> taskQueue_;

};

} /* Thread */

#endif /* TaskQueueThread_h */
