#ifndef SocketPollThread_h
#define SocketPollThread_h

#include "Thread.h"
#include "TaskQueue.h"
#include "SocketTasker.h"
#include <sys/epoll.h>
#include <unordered_map>
#include <sys/eventfd.h>

namespace Threading {

static constexpr uint64_t incrementer_ = 1;

class SocketPollThread : public Thread {
public:
    class CrossThreadSignal : public SocketTasker {
    public:
        CrossThreadSignal()
            : SocketTasker(static_cast<FileDescriptor>(eventfd(0, EFD_SEMAPHORE))) { }

        void run() override
        {
            auto count = queue_.countAsReader();
            std::vector<Signals> items(count);
            queue_.popChunk(items, count);
            for (auto& signal : items) {
                switch (signal) {
                    case Signals::Kill:
                        DEBUG("Got Kill signal");
                        thisThread()->kill();
                        break;
                }
            }
        }

        // TODO I don't really like this, we should do something better.
        // "this" may not exist here... Maybe do some checking?
        std::function<void(Signals)> notifier()
        {
            return [this] (Signals signal) {
                queue_.push(signal);
                write(socket_, &incrementer_, sizeof(uint64_t));
            };
        }

    private:
        TaskQueue<Signals, 255> queue_;

    };

    class SocketPoolSingleTasker : public SocketTasker {
    public:
        SocketPoolSingleTasker()
            : SocketTasker(static_cast<FileDescriptor>(eventfd(0, EFD_SEMAPHORE))) { }

        void run() override
        {
            auto count = queue_.countAsReader();
            std::vector<std::unique_ptr<Tasker>> items(count);
            queue_.popChunk(items, count);
            for (auto& task : items) {
                task->run();
            }
        }

        // TODO I don't really like this, we should do something better.
        // "this" may not exist here... Maybe do some checking?
        std::function<void(std::unique_ptr<Tasker>)> notifier()
        {
            return [this] (std::unique_ptr<Tasker> task) {
                queue_.push(std::move(task));
                write(socket_, &incrementer_, sizeof(uint64_t));
            };
        }

    private:
        TaskQueue<std::unique_ptr<Tasker>, 255> queue_;

    };

    SocketPollThread(std::unique_ptr<std::thread>, const std::string& name);

    void addSocketTasker(std::unique_ptr<SocketTasker>);
    void addTask(std::unique_ptr<Tasker>);
    void kill() override;
    void entryPoint() override;
    void sendSignal(Signals) override;

private:
    void handleEvent_(epoll_event);
    void checkBuffers_();

    FileDescriptor epollfd_;
    // TODO This is not thread-safe.
    std::mutex map_lock_;
    std::unordered_map<FileDescriptor, std::unique_ptr<SocketTasker>> file_descriptor_tasker_map_;
    bool running_;

    std::function<void(Signals)> cross_thread_notifier_;
    std::function<void(std::unique_ptr<Tasker>)> cross_thread_task_runner_adder_;

};

} /* Thread */

#endif /* SocketPollThread_h */
