#include "base/Allocator.h"
#include "net/sockets/EventSocket.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include "base/Allocator.h"

using namespace net;

EventSocket::EventSocket()
    : current_value_(0)
{
    socket_ = static_cast<SocketEventDeligate::FileDescriptorId>(eventfd(0, EFD_NONBLOCK));
    ASSERT(socket_ == -1, "eventfd() call failed");
}

void EventSocket::triggerEvent()
{
    write(static_cast<int>(socket_), &current_value_, sizeof(current_value_));
}
