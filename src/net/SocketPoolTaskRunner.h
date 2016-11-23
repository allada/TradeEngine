#ifndef SocketPoolTaskRunner_h
#define SocketPoolTaskRunner_h

#include "sockets/SocketEventDeligate.h"
#include "sockets/EventSocket.h"

namespace net {
namespace SocketPoolTaskRunner {
    enum ThreadSignals {
        Terminate
    };

    void addSocket(SocketEventDeligate*);
    void start();
    void terminate();
} /* SocketPoolTaskRunner */
} /* net */

#endif /* SocketPoolTaskRunner_h */