#ifndef UDPSocket_h
#define UDPSocket_h

#include <sys/types.h>
#include <sys/socket.h>
#include "net/sockets/SocketEventDeligate.h"

namespace net {

class UDPSocket : virtual public SocketEventDeligate {
public:
    SocketEventDeligate::FileDescriptorId fileDescriptor() override { return socket_; }
    void shouldProcessEvent(const std::vector<char> &) override { }
    void processEvent() override { }

protected:
    SocketEventDeligate::FileDescriptorId socket_;
};

} /* net */

#endif /* UDPSocket_h */