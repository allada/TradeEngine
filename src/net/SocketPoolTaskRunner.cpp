#include "base/Allocator.h"
#include "base/ThreadManager.h"
#include "SocketPoolTaskRunner.h"
#include <string.h>
#include <sys/epoll.h>
#include <unordered_map>

#define MAXEVENTS 0xFF

using namespace net;
using namespace SocketPoolTaskRunner;

void ioLoop(); 
void uiLoop();
void closeIoThread();
void closeUiThread();

SocketEventDeligate::FileDescriptorId epollfd_(static_cast<int>(epoll_create1(0)));

bool have_data_ = false;

// TODO Make lock free queue class.
// We always offset the reader and writer by 1 to ensure end and beginning cases are not an issue.
Closure ui_task_queue_[MAXEVENTS];
uint8_t queue_read_position_ = 0;
uint8_t queue_write_position_ = MAXEVENTS;

std::unordered_map<SocketEventDeligate::FileDescriptorId, SocketEventDeligate*> descriptors_;

EventSocket<ThreadSignals> io_thread_chanel(false);
EventSocket<bool> ui_thread_chanel(true);


ThreadManager::Thread thread_io_(ioLoop);
ThreadManager::Thread thread_ui_(uiLoop);

bool runIoThread = true;
bool runUiThread = true;

void SocketPoolTaskRunner::start()
{
    io_thread_chanel.setIoThreadHandler(std::bind(&closeIoThread));
    io_thread_chanel.setUiThreadHandler(std::bind(&closeUiThread));

    addSocket(&io_thread_chanel);
}

void SocketPoolTaskRunner::terminate()
{
    io_thread_chanel.data = Terminate;
    io_thread_chanel.triggerEvent();
    thread_io_.join();
    thread_ui_.join();
    DEBUG("Threads closed");
}

void closeIoThread()
{
    runIoThread = false;
}

void closeUiThread()
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
        WARNING("Error epoll_ctl()");

    descriptors_.emplace(std::make_pair(descriptor->fileDescriptor(), descriptor));
}

void ioLoop()
{
    //TerminalColor::registerThread(TerminalColor::GREEN);
    std::array<epoll_event, MAXEVENTS> events{};
    while (runIoThread) {
        DEBUG("Started IoThread");
        int len = epoll_wait(epollfd_, events.data(), events.size(), -1);
        switch (len) {
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
        }
        for (int i = 0; i < len; ++i) {
            epoll_event event(events[i]);
            if (event.events & EPOLLERR) {
                WARNING("Socket Error");
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
            SocketEventDeligate::FileDescriptorId fileDescriptor =
                    static_cast<SocketEventDeligate::FileDescriptorId>(event.data.fd);
            SocketEventDeligate* deligate = descriptors_.at(fileDescriptor);
            // TODO: Assert deligate.
            if (deligate->shouldRunIoHandler())
                deligate->ioThreadHandler();
            if (deligate->shouldRunUiHandler()) {
                uint8_t local_read_position;
                // Will yield to the uiLoop before we add more onto the queue.
                for(;;) {
                    local_read_position = queue_read_position_;
                    local_read_position = (local_read_position - 1) % MAXEVENTS;
                    if (local_read_position != queue_write_position_)
                        std::this_thread::yield();
                    else
                        break;
                }

                ++queue_write_position_;
                queue_write_position_ %= MAXEVENTS;
                ui_task_queue_[queue_write_position_] = std::bind(&SocketEventDeligate::uiThreadHandler, deligate);
                have_data_ = true;
                ui_thread_chanel.triggerEvent();
            }
        }
    }
    DEBUG("Closed IO Thread");
}

void uiLoop()
{
    //TerminalColor::registerThread(TerminalColor::BLUE);
    while (runUiThread) {
        DEBUG("Started UiThread");
        ui_thread_chanel.wait();
        if (!have_data_)
            continue;
        uint8_t local_write_position = queue_write_position_ + 1;
        while (local_write_position != queue_read_position_) {
            ui_task_queue_[queue_read_position_]();
            ++queue_read_position_;
            queue_read_position_ %= MAXEVENTS;
        }
        have_data_ = false;
    }
    DEBUG("Closed UI Thread");
}

void handleData(const std::vector<char> &data)
{

}
