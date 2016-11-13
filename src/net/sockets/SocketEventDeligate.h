#ifndef SocketDeligate_h
#define SocketDeligate_h

namespace net {

class SocketEventDeligate {
public:
    using FileDescriptorId = int;

    /* Is ran on IO thread */
    virtual FileDescriptorId fileDescriptor();

    /* Is ran on IO thread. Should be fast and only check if data should be passed for processing. */
    virtual void shouldProcessEvent(const std::vector<char> &data);

    /* Is ran on UI thread. */
    virtual void processEvent(); 
};

}; /* net */

#endif /* SocketDeligate_h */