#include "src/net/sockets/DataGramSocket.h"
#include "src/conf.h"

using namespace net;

DataGramSocket::DataGramSocket()
 : socket_(socket(AF_INET, SOCK_DGRAM, 0))
{
    ASSERT(socket_ == -1, "socket() call failed");
}
/*
void DataGramSocket::broadcast(const char* message, size_t len)
{
    sockaddr_in sock_info;
    sock_info.sin_addr.s_addr = inet_addr(BROADCAST_GROUP);
    sock_info.sin_port = htons(BROADCAST_PORT);

    sendto(sock_, message, len, MSG_DONTWAIT, (struct sockaddr *) &addr, sizeof(addr));
}*/