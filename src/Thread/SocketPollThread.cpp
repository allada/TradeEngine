#include "SocketPollThread.h"
#include "Threader.h"
#include "../Common.h"

using namespace Thread;

static const uint8_t MAXEVENTS = 0xFF;

SocketPollThread::SocketPollThread(std::unique_ptr<std::thread> thread, const std::string& name)
    : Thread(std::move(thread), name)
    , epollfd_(static_cast<int>(epoll_create1(0)))
    , running_(true)
{
    std::unique_ptr<CrossThreadSignal> signaler = std::unique_ptr<CrossThreadSignal>(new CrossThreadSignal);
    cross_thread_notifier_ = signaler->notifier();
    addSocketTasker(std::move(signaler));
}

void SocketPollThread::addSocketTasker(std::unique_ptr<SocketTasker> socketTasker)
{
    ASSERT_NE(epollfd_, -1, "epoll_create1() must be valid");

    struct epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = static_cast<int>(socketTasker->fileDescriptor());
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        WARNING("Error epoll_ctl()");
    }
    file_descriptor_tasker_map_.emplace(ev.data.fd, std::move(socketTasker));
}

void SocketPollThread::kill()
{
    if (id() == thisThreadId()) {
        DEBUG("Got shutdown command");
        running_ = false;
    } else {
        DEBUG("Killing thread: %s", this->name().c_str());
        sendSignal(Signals::Kill);
    }
}

inline int checkEpollWaitResult(int result)
{
    switch (result) {
    case EBADF:
        WARNING("epfd is not a valid file descriptor.");
        return 0;
    case EFAULT:
        WARNING("The memory area pointed to by events is not accessible with write permissions.");
        return 0;
    case EINTR:
        WARNING("The call was interrupted by a signal handler before either (1) any of the requested events "
                "occurred or (2) the timeout expired;");
        return 0;
    case EINVAL:
        WARNING("epfd is not an epoll file descriptor, or maxevents is less than or equal to zero.");
        return 0;
    }
    return result;
}

void SocketPollThread::entryPoint()
{
    std::array<epoll_event, MAXEVENTS> events{};
    while (running_) {
        int len = checkEpollWaitResult(epoll_wait(epollfd_, events.data(), events.size(), -1));
        for (int i = 0; i < len && running_; ++i) {
            handleEvent_(events[i]);
        }
    }
    DEBUG("Exited thread loop");
}

void SocketPollThread::handleEvent_(epoll_event event)
{
    ASSERT_EQ((event.events & EPOLLIN), EPOLLIN, "Socket should be EPOLLIN.");

    FileDescriptor fd = static_cast<FileDescriptor>(event.data.fd);
    if (event.events & EPOLLERR) {
        // TODO better errors.
        file_descriptor_tasker_map_.at(fd)->error();
    }
    if (event.events & EPOLLHUP) {
        // TODO better hangup.
        file_descriptor_tasker_map_.erase(fd);
    }
    file_descriptor_tasker_map_.at(fd)->run();
}

void SocketPollThread::sendSignal(Signals signal)
{
    DEBUG("Sending %d signal", signal);
    cross_thread_notifier_(signal);
}
