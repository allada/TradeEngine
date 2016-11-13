#include "net/sockets/EventSocket.h"
#include <sys/eventfd.h>
#include "base/Allocator.h"

using namespace net;

EventSocket::EventSocket()
    : socket_(static_cast<FileDescriptor::FileDescriptorId>(eventfd(0, EFD_NONBLOCK)))
    , current_value_(0)
{
    ASSERT(socket_ == -1, "eventfd() call failed");
}

void EventSocket::triggerEvent()
{
    write(static_cast<int>(socket_), current_value_, sizeof(current_value_));
}
