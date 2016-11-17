#ifndef SocketEventDeligate_h
#define SocketEventDeligate_h

#include <vector>

namespace net {

class SocketEventDeligate {
public:
    SocketEventDeligate() { }
    using FileDescriptorId = int;

    /* Is ran on IO thread */
    virtual FileDescriptorId fileDescriptor() const = 0;

    /* Is ran on IO thread. Should be fast and only check if data should be passed for processing. */
    virtual void shouldProcessEvent(const std::vector<char> &data) = 0;

    /* Is ran on UI thread. */
    virtual void processEvent() = 0;
};

} /* net */

#endif /* SocketEventDeligate_h */