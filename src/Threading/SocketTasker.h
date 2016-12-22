#ifndef SocketTasker_h
#define SocketTasker_h

#include "Tasker.h"
#include <unistd.h>

namespace Threading {

class SocketTasker : public virtual Tasker {
public:
    SocketTasker(FileDescriptor socket)
        : socket_(socket)
    {
        EXPECT_NE(socket, -1);
    }

    ~SocketTasker() override
    {
        this->close();
    }

    FileDescriptor fileDescriptor() { return socket_; }
    void close() {
        DEBUG("Socket %d closed", socket_);
        ::close(socket_);
    }

    void error() {
        // TODO Finish.
    }

    void checkBuffer()
    {
        run();
    }

    virtual void run() override = 0;

protected:
    FileDescriptor socket_;

};

} /* Thread */

#endif /* SocketTasker_h */
