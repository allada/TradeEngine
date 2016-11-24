#include "QueueThread.h"
#include "Threader.h"
#include "../Common.h"

using namespace Thread;

static const uint8_t MAXEVENTS = 0xFF;

QueueThread::QueueThread(std::unique_ptr<std::thread> thread, const std::string& name)
    : Thread(std::move(thread), name)
    , epollfd_(static_cast<int>(epoll_create1(0)))
    , thread_signal_channel_(std::bind(&QueueThread::handleThreadSignal_, this, std::placeholders::_1))
    , running_(true)
{
    addTaskChannel(&thread_signal_channel_);
}

void QueueThread::addTaskChannel(CrossThreadEventer* event)
{
    ASSERT(epollfd_ == -1, "epoll_create1() must be valid");

    struct epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = static_cast<int>(event->fileDescriptor());
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
        WARNING("Error epoll_ctl()");

    thread_event_map_.emplace(std::make_pair(event->fileDescriptor(), event));
}

void QueueThread::kill()
{
    if (id() == thisThreadId()) {
        DEBUG("Got shutdown command");
        running_ = false;
    } else {
        DEBUG("Killing thread: %s", this->name().c_str());
        thread_signal_channel_.send(Kill);
    }
}

inline int checkEpollWaitResult(int result)
{
    switch (result) {
    case EBADF:
        WARNING("epfd is not a valid file descriptor.");
        break;
    case EFAULT:
        WARNING("The memory area pointed to by events is not accessible with write permissions.");
        break;
    case EINTR:
        WARNING("The call was interrupted by a signal handler before either (1) any of the requested events "
                "occurred or (2) the timeout expired;");
        break;
    case EINVAL:
        WARNING("epfd is not an epoll file descriptor, or maxevents is less than or equal to zero.");
        break;
    default:
        // Everything is ok.
        return result;
    }
    return 0;
}

void QueueThread::entryPoint()
{
    std::array<epoll_event, MAXEVENTS> events{};
    while (running_) {
        int len = checkEpollWaitResult(epoll_wait(epollfd_, events.data(), events.size(), -1));

        for (int i = 0; i < len; ++i) {
            ThreadEventer* threadEvent = threadEventFromFileDescriptor_(events[i]);
            threadEvent->run();
            if (!running_)
                break;
        }
    }
    DEBUG("Exited thread loop");
}

ThreadEventer* QueueThread::threadEventFromFileDescriptor_(epoll_event event)
{
    ThreadEventer* threadEvent = thread_event_map_.at(static_cast<FileDescriptor>(event.data.fd));
    // TODO Assert threadEvent?

    if (event.events & EPOLLERR) {
        WARNING("Socket Error");
        threadEvent->error(EPOLLERR);
        return nullptr;
    }
    if (event.events & EPOLLHUP) {
        threadEvent->close();
        return nullptr;
    }
    ASSERT_EQ((event.events & EPOLLIN), EPOLLIN, "Socket should be EPOLLIN.");
    return threadEvent;
}

void QueueThread::handleThreadSignal_(Signals signal) {
    ASSERT_EQ(id(), thisThreadId(), "handleThreadSignal_ may only be invoked from it's own thread.");
    switch (signal) {
    case Kill:
        kill();
        break;
    }
}
