#ifndef UDPSocket_h
#define UDPSocket_h

#include <sys/types.h>
#include <sys/socket.h>
#include "net/sockets/SocketEventDeligate.h"

namespace net {

class UDPSocket : public SocketEventDeligate {
public:
    SocketEventDeligate::FileDescriptorId fileDescriptor() const override { return socket_; }
    void shouldProcessEvent(const std::vector<char> &) override { }

protected:
    SocketEventDeligate::FileDescriptorId socket_;
};

} /* net */

#endif /* UDPSocket_h */