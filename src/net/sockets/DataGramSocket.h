#ifndef DataGramSocket_h
#define DataGramSocket_h

#include "net/sockets/UDPSocket.h"

namespace net {

class DataGramSocket : public UDPSocket {
public:
    DataGramSocket();

    //void broadcast(const char*, size_t);
};

} /* net */

#endif /* DataGramSocket_h */