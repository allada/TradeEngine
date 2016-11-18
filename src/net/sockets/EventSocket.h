#ifndef EventSocket_h
#define EventSocket_h

#include "net/sockets/SocketEventDeligate.h"
#include "base/Allocator.h"

namespace net {

template <class T>
class EventSocket : public SocketEventDeligate {
public:
    EventSocket(bool);

    SocketEventDeligate::FileDescriptorId fileDescriptor() const override { return socket_; }
    void shouldProcessEvent(const std::vector<char> &) override { }

    bool shouldRunIoHandler() override { return hasIoThreadHandler; }
    void ioThreadHandler() override;
    bool shouldRunUiHandler() override { return hasUiThreadHandler; }
    void uiThreadHandler() override;

    void setIoThreadHandler(const Closure& handler) {
        hasIoThreadHandler = true;
        ioThreadHandler_ = handler;
    }
    void setUiThreadHandler(const Closure& handler) {
        hasUiThreadHandler = true;
        uiThreadHandler_ = handler;
    }

    void triggerEvent();
    void wait();

    T data;
protected:
    bool hasIoThreadHandler;
    bool hasUiThreadHandler;
    Closure ioThreadHandler_;
    Closure uiThreadHandler_;
    SocketEventDeligate::FileDescriptorId socket_;
};

} /* net */

// This is needed to prevent linking issues with template classes.
#include "EventSocket.cpp"
#endif /* EventSocket_h */