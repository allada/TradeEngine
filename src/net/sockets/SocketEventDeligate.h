#ifndef SocketEventDeligate_h
#define SocketEventDeligate_h

#include <vector>

namespace net {

class SocketEventDeligate {
public:
    using FileDescriptorId = int;

    /* Is ran on IO thread */
    virtual FileDescriptorId fileDescriptor() const = 0;

    /* Is ran on IO thread. Should be fast and only check if data should be passed for processing. */
    virtual void shouldProcessEvent(const std::vector<char> &data) = 0;

    /* If the uiThreadHandler should be ran */
    virtual bool shouldRunUiHandler() { return false; }

    /* Is ran on UI thread. */
    virtual void uiThreadHandler() { }

    /* If the ioThreadHandler should be ran */
    virtual bool shouldRunIoHandler() { return false; }

    /* Is ran on IO thread */
    virtual void ioThreadHandler() { }
};

} /* net */

#endif /* SocketEventDeligate_h */