#include "base/Allocator.h"
#include "net/sockets/EventSocket.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include "base/Allocator.h"

using namespace net;

static uint64_t dummyData = 1;

template <class T>
EventSocket<T>::EventSocket(const Closure& handler)
    : handler_(handler)
{
    socket_ = static_cast<SocketEventDeligate::FileDescriptorId>(eventfd(0, EFD_NONBLOCK));
    ASSERT(socket_ == -1, "eventfd() call failed");
}

template <class T>
void EventSocket<T>::triggerEvent()
{
    // TODO maybe remove this?
    int len = write(static_cast<int>(socket_), &dummyData, sizeof(dummyData));

}

template <class T>
void EventSocket<T>::processEvent()
{
    // TODO: Assert thread
    handler_();
}
