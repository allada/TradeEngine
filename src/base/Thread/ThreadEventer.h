#ifndef ThreadEventer_h
#define ThreadEventer_h

#include "../Common.h"

namespace Thread {

class ThreadEventer {
public:
    virtual FileDescriptor fileDescriptor() = 0;
    virtual void notify() = 0;
    virtual void wait() = 0;
    virtual void run() = 0;
    virtual void close() = 0;
    virtual void error(int err) = 0;
};

} /* Thread */

#endif /* ThreadEventer_h */