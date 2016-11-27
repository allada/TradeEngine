#ifndef QueueThread_h
#define QueueThread_h

#include "Thread.h"
#include "ThreadQueue.h"
#include "CrossThreadEventer.h"
#include <sys/epoll.h>
#include <unordered_map>

namespace Thread {

class QueueThread : public Thread<QueueThread> {
public:
    template <class T>
    class CrossThreadEvent : public CrossThreadEventer {
    public:
        CrossThreadEvent(std::function<void(T)> signal_handler)
        {
signal_handler_ = signal_handler;
        }

        ~CrossThreadEvent()
        {
            clean_(std::is_pointer<T>());
            //TODO FINISH!
        }

        void send(T item)
        {
            queue_.push(item);
            notify();
        }

        void run() override
        {
            int len = queue_.count();
            for (int i = 0; i < len; ++i) {
                T item = queue_.pop();
                signal_handler_(item);
            }
        }

    private:
        // Runs this if T is a pointer
        void clean_(std::true_type)
        {
            int len = queue_.count();
            for (int i = 0; i < len; ++i) {
                queue_.pop();
            }
        }

        // Runs this if T is NOT a pointer
        void clean_(std::false_type) { }

        ThreadQueue<T> queue_;
        std::function<void(T)> signal_handler_;
    };

    enum Signals {
        Kill
    };

    QueueThread(std::unique_ptr<std::thread>, const std::string& name);

    // TODO FINISH
    // Memory leaks
    void addTaskChannel(CrossThreadEventer*);
    void kill() override;
    void entryPoint() override;
private:
    inline ThreadEventer* threadEventFromFileDescriptor_(epoll_event);
    void handleThreadSignal_(Signals);

    FileDescriptor epollfd_;
    CrossThreadEvent<Signals> thread_signal_channel_;

    std::unordered_map<FileDescriptor, ThreadEventer*> thread_event_map_;
    bool running_;
};

} /* Thread */

#endif /* QueueThread_h */
