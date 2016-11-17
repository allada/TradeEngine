#include "base/Allocator.h"
#include "SocketPoolTaskRunner.h"
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#define MAXEVENTS 0xF

using namespace net;
using namespace SocketPoolTaskRunner;

void ioLoop(); 
void uiLoop();
void checkIoSignal();
void checkUiSignal();

SocketEventDeligate::FileDescriptorId epollfd_(static_cast<int>(epoll_create1(0)));

std::mutex ui_task_queue_lock_;
std::condition_variable ui_task_event_;
std::queue<Closure> ui_task_queue_;

std::unordered_map<SocketEventDeligate::FileDescriptorId, SocketEventDeligate*> descriptors_;

EventSocket<ThreadSignals> io_thread_chanel(std::bind(&checkIoSignal));
EventSocket<ThreadSignals> ui_thread_chanel(std::bind(&checkUiSignal));

std::thread thread_io_(ioLoop);
std::thread thread_ui_(uiLoop);

bool runIoThread = true;
bool runUiThread = true;

void SocketPoolTaskRunner::start()
{
    addSocket(&io_thread_chanel);
}

void SocketPoolTaskRunner::terminate()
{
    io_thread_chanel.data = Terminate;
    io_thread_chanel.triggerEvent();
    thread_io_.join();
    thread_ui_.join();
}

void checkIoSignal()
{
    runIoThread = false;
}

void checkUiSignal()
{
    runUiThread = false;
}

void SocketPoolTaskRunner::addSocket(SocketEventDeligate* descriptor)
{
    ASSERT(epollfd_ == -1, "epoll_create1() call failed");

    struct epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = static_cast<int>(descriptor->fileDescriptor());
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
        std::fprintf(stderr, "Error epoll_ctl()\n");

    descriptors_.emplace(std::make_pair(descriptor->fileDescriptor(), descriptor));
}

void ioLoop()
{
    std::array<epoll_event, MAXEVENTS> events{};
    while (runIoThread) {
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
            SocketEventDeligate::FileDescriptorId fileDescriptor = static_cast<SocketEventDeligate::FileDescriptorId>(event.data.fd);
            SocketEventDeligate* handler = descriptors_.at(fileDescriptor);
            // TODO: Assert deligate.
            handler->processEvent();
            //ui_task_queue_.push(std::bind(&handleData, ));
        }
    }
    ui_task_event_.notify_one();
}

void uiLoop()
{

}

void handleData(const std::vector<char> &data)
{

}
