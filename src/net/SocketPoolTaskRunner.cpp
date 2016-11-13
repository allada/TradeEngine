#include "SocketPoolTaskRunner.h"
#include "base/Allocator.h"
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

#define MAXEVENTS 0xF

using namespace net;

void SocketPoolTaskRunner::start()
{
    //event_socket_writer_ = WrapUnique(new EventSocket());
    //this.addSocket(event_socket_writer_.clone());
    epollfd_ = static_cast<int>(epoll_create1(0));

    for (auto&& descriptor : descriptors_) {
        ASSERT(epollfd_ == -1, "create_reactor() call failed");

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = static_cast<int>(descriptor.second->fileDescriptor());

        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
            std::fprintf(stderr, "Error epoll_ctrl()\n");
    }

    thread_io_ = WrapUnique(new std::thread(SocketPoolTaskRunner::ioLoop));
    thread_ui_ = WrapUnique(new std::thread(SocketPoolTaskRunner::uiLoop));
}

void SocketPoolTaskRunner::addSocket(std::unique_ptr<SocketEventDeligate> descriptor)
{
    ASSERT(epollfd_ == -1, "create_reactor() call failed");

    struct epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = static_cast<int>(descriptor->fileDescriptor());
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
        std::fprintf(stderr, "Error epoll_ctl()\n");

    descriptors_.emplace(std::make_pair(descriptor->fileDescriptor(), std::move(descriptor)));
}

void SocketPoolTaskRunner::ioLoop()
{
    std::array<epoll_event, MAXEVENTS> events{};
    for (;;) {
        bool served_data = false;
        int len = epoll_wait(epollfd_, events.data(), events.size(), -1);
        switch (len) {
        case EBADF:
            fprintf(stderr, "epfd is not a valid file descriptor.\n");
            break;
        case EFAULT:
            fprintf(stderr, "The memory area pointed to by events is not "
                            "accessible with write permissions.\n");
            break;
        case EINTR:
            fprintf(stderr, "The call was interrupted by a signal handler "
                            "before either (1) any of the requested events "
                            "occurred or (2) the timeout expired;");
            break;
        case EINVAL:
            fprintf(stderr, "epfd is not an epoll file descriptor, or "
                            "maxevents is less than or equal to zero.");
            break;
        }
        for (int i = 0; i < len; ++i) {
            epoll_event event(events[i]);
            if (event.events & EPOLLERR) {
                fprintf(stderr, "Socket Error\n");
                close(event.data.fd);
                continue;
            }
            if (event.events & EPOLLHUP) {
                close(event.data.fd);
                continue;
            }
            if ((event.events & EPOLLIN) != EPOLLIN) {
                ASSERT(true, "Socket should be EPOLLIN.");
                close(event.data.fd);
                continue;
            }
            //ui_task_queue_.push(std::bind(&handleData, ));
            served_data = true;
        }
    }
    ui_task_event_.notify_one();
}

void SocketPoolTaskRunner::uiLoop()
{

}

void handleData(const std::vector<char> &data)
{

}
