#ifndef SocketPollThread_h
#define SocketPollThread_h

#include "Thread.h"
#include "TaskQueue.h"
#include "SocketTasker.h"
#include <sys/epoll.h>
#include <unordered_map>

namespace Thread {

static constexpr uint64_t incrementer_ = 1;

class SocketPollThread : public Thread<SocketPollThread> {
public:
    class CrossThreadSignal : public SocketTasker {
    public:
        void run() override
        {
            auto count = queue_.countAsReader();
            for (auto i = count; i > 0; --i) {
                const Signals signal = queue_.pop();
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
                write(socket_, &incrementer_, sizeof(incrementer_));
            };
        }

    private:
        TaskQueue<Signals> queue_;

    };

    SocketPollThread(std::unique_ptr<std::thread>, const std::string& name);

    void addSocketTasker(std::unique_ptr<SocketTasker>);
    void kill() override;
    void entryPoint() override;
    void sendSignal(Signals) override;

private:
    inline void handleEvent_(epoll_event);

    FileDescriptor epollfd_;
    std::unordered_map<FileDescriptor, std::unique_ptr<SocketTasker>> file_descriptor_tasker_map_;
    bool running_;

    std::function<void(Signals)> cross_thread_notifier_;

};

} /* Thread */

#endif /* SocketPollThread_h */
