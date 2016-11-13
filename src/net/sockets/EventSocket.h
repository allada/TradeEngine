#ifndef EventSocket_h
#define EventSocket_h

#include "net/sockets/SocketEventDeligate.h"
#include "base/Allocator.h"

namespace net {

class EventSocket : public SocketEventDeligate {
public:
    SocketEventDeligate::FileDescriptorId fileDescriptor() override { return socket_; }

    std::unique_ptr<EventSocket> copy()
    {
        return WrapUnique(new EventSocket(this));
    }

protected:
    SocketEventDeligate::FileDescriptorId socket_;
    uint64_t current_value_;

private:
    EventSocket(EventSocket* event_socket) : current_value_(event_socket->fileDescriptor()) { }
};

} /* net */

#endif /* EventSocket_h */