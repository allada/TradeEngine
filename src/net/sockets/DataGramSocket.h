#ifndef DataGramSocket_h
#define DataGramSocket_h

#include "src/net/sockets/UDPSocket.h"

using namespace net;

class DataGramSocket : public UDPSocket {
public:
    DataGramSocket();

    //void broadcast(const char*, size_t);
}

#endif /* DataGramSocket_h */