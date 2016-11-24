#ifndef CrossThreadEventer_h
#define CrossThreadEventer_h

#include "ThreadEventer.h"
#include "../Common.h"
#include <sys/eventfd.h>

namespace Thread {

class CrossThreadEventer : public virtual ThreadEventer {
public:
    CrossThreadEventer();

    FileDescriptor fileDescriptor() override { return socket_; }
    void notify() override;
    void wait() override;

    void close() override;
    void error(int err) override;

    virtual void run() override = 0;

private:
    FileDescriptor socket_;
};

} /* Thread */

#endif /* CrossThreadEventer_h */