#ifndef EventSocket_h
#define EventSocket_h

#include "net/sockets/SocketEventDeligate.h"
#include "base/Allocator.h"

namespace net {

template <class T>
class EventSocket : virtual public SocketEventDeligate {
public:
    EventSocket(const Closure&);
    SocketEventDeligate::FileDescriptorId fileDescriptor() const override { return socket_; }
    void shouldProcessEvent(const std::vector<char> &) override { }
    void processEvent() override;

    void triggerEvent();

    T data;
protected:
    Closure handler_;
    SocketEventDeligate::FileDescriptorId socket_;
};

} /* net */

// This is needed to prevent linking issues with template classes.
#include "EventSocket.cpp"
#endif /* EventSocket_h */