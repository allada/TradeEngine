#ifndef SocketPoolTaskRunner_h
#define SocketPoolTaskRunner_h

#include <condition_variable>
#include <thread>
#include <queue>
#include <unordered_map>
#include "sockets/SocketEventDeligate.h"
#include "sockets/EventSocket.h"

namespace net {

class SocketPoolTaskRunner {
    STATIC_ONLY(SocketPoolTaskRunner);
public:
    static void addSocket(std::unique_ptr<SocketEventDeligate> descriptor);

    static void start();

private:
    static void ioLoop();
    static void uiLoop();

    static SocketEventDeligate::FileDescriptorId epollfd_;

    static std::mutex ui_task_queue_lock_;
    static std::condition_variable ui_task_event_;
    static std::queue<Closure> ui_task_queue_;

    static std::unordered_map<SocketEventDeligate::FileDescriptorId, std::unique_ptr<SocketEventDeligate>> descriptors_;

    static std::unique_ptr<EventSocket> event_socket_writer_;

    static std::unique_ptr<std::thread> thread_io_;
    static std::unique_ptr<std::thread> thread_ui_;
};

} /* net */

#endif /* SocketPoolTaskRunner_h */