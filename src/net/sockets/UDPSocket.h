#ifndef UDPSocket_h
#define UDPSocket_h

#include <sys/types.h>
#include <sys/socket.h>
#include "net/sockets/SocketEventDeligate.h"

namespace net {

class UDPSocket : public SocketEventDeligate {
public:
    FileDescriptor::FileDescriptorId fileDescriptor() override { return socket_; }

protected:
    FileDescriptor::FileDescriptorId socket_;
};

}; /* net */

#endif /* UDPSocket_h */