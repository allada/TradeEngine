#include "SocketPollThread.h"
#include "Threader.h"
#include "../Common.h"
#include <cstring>

using namespace Threading;

static const uint8_t MAXEVENTS = 0xFF;

SocketPollThread::SocketPollThread(std::unique_ptr<std::thread> thread, const std::string& name)
    : Thread(std::move(thread), name)
    , epollfd_(static_cast<int>(epoll_create1(0)))
    , running_(true)
{
    std::unique_ptr<CrossThreadSignal> signaler = std::unique_ptr<CrossThreadSignal>(new CrossThreadSignal);
    std::unique_ptr<SocketPoolSingleTasker> singleTaskRunner = std::unique_ptr<SocketPoolSingleTasker>(new SocketPoolSingleTasker);
    
    cross_thread_notifier_ = signaler->notifier();
    cross_thread_task_runner_adder_ = singleTaskRunner->notifier();
    addSocketTasker(std::move(signaler));
    addSocketTasker(std::move(singleTaskRunner));
}

void SocketPollThread::addSocketTasker(std::unique_ptr<SocketTasker> socketTasker)
{
    std::lock_guard<std::mutex> lock(map_lock_);
    EXPECT_NE(epollfd_, -1);

    struct epoll_event ev;
    memset(&ev, '\0', sizeof(ev));
    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = static_cast<int>(socketTasker->fileDescriptor());
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        WARNING("Error epoll_ctl()");
    }
    file_descriptor_tasker_map_.emplace(static_cast<FileDescriptor>(ev.data.fd), std::move(socketTasker));
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
        int len = checkEpollWaitResult(epoll_wait(epollfd_, events.data(), events.size(), 10));
        {
            std::lock_guard<std::mutex> lock(map_lock_);
            if (len == 0) {
                for (int i = 0; i < len && running_; ++i) {
                    checkBuffers_();
                }
            }
            for (int i = 0; i < len && running_; ++i) {
                handleEvent_(events[i]);
            }
        }
    }
    DEBUG("Exited thread loop");
}

void SocketPollThread::checkBuffers_()
{
    for (auto& task : file_descriptor_tasker_map_) {
        task.second->checkBuffer();
    }
}

void SocketPollThread::handleEvent_(epoll_event event)
{
    EXPECT_EQ(size_t(event.events & EPOLLIN), size_t(EPOLLIN));

    FileDescriptor fd = static_cast<FileDescriptor>(event.data.fd);
    if (event.events & EPOLLERR) {
        // TODO better errors.
        file_descriptor_tasker_map_.at(fd)->error();
    }
    if (event.events & EPOLLHUP) {
        // TODO better hangup.
        file_descriptor_tasker_map_.erase(fd);
        return;
    }
    file_descriptor_tasker_map_.at(fd)->run();

}

void SocketPollThread::sendSignal(Signals signal)
{
    cross_thread_notifier_(signal);
}
