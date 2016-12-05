#ifndef SocketTasker_h
#define SocketTasker_h

#include "Tasker.h"
#include <unistd.h>

namespace Thread {

class SocketTasker : virtual public Tasker {
public:
    SocketTasker(FileDescriptor socket)
        : socket_(socket)
    {
        DEBUG("Socket %d created", socket_);
    }

    ~SocketTasker() { this->close(); }

    FileDescriptor fileDescriptor() { return socket_; }
    void close() {
        DEBUG("Socket %d closed", socket_);
        ::close(socket_);
    }

    void error() {
        // TODO Finish.
    }

    virtual void run() = 0;

protected:
    FileDescriptor socket_;

};

} /* Thread */

#endif /* SocketTasker_h */
