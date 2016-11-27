#include "base/Allocator.h"
#include "net/sockets/EventSocket.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include "base/Allocator.h"

using namespace net;

static uint64_t dummyData = 1;

template <class T>
EventSocket<T>::EventSocket(bool shouldBlock)
{
    socket_ = static_cast<SocketEventDeligate::FileDescriptorId>(eventfd(0, (shouldBlock ? 0 : EFD_NONBLOCK) | EFD_SEMAPHORE));
    ASSERT(socket_ == -1, "eventfd() call failed");
}

template <class T>
void EventSocket<T>::triggerEvent()
{
    write(static_cast<int>(socket_), &dummyData, sizeof(dummyData));
}

template <class T>
void EventSocket<T>::ioThreadHandler()
{
    // TODO: Assert thread
    ioThreadHandler_();
}

template <class T>
void EventSocket<T>::uiThreadHandler()
{
    // TODO: Assert thread
    uiThreadHandler_();
}

template <class T>
void EventSocket<T>::wait()
{
    read(socket_, nullptr, sizeof(uint64_t));
}
