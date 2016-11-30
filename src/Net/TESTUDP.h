#ifndef TESTUDP_h
#define TESTUDP_h

#include "../Thread/Tasker.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


// TODO Move this config data somewhere else.
#define SERV_PORT 3010
#define MAX_BUFF_SIZE 2048

namespace Net {

class TESTUDP : public virtual Thread::Tasker {
public:
    TESTUDP()
        : socket_(static_cast<FileDescriptor>(socket(AF_INET, SOCK_DGRAM, 0)))
    {
        servAddr_.sin_family = AF_INET;
        servAddr_.sin_addr.s_addr = inet_addr("127.255.255.255");
        servAddr_.sin_port = htons(SERV_PORT);

        const int broadcast = 1;
        setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
        DEBUG("UDP sender %d ready", socket_);
    }

    void run() override
    {
        unsigned char buff[] { '1', '2', '3' };
        auto len = sendto(socket_, &buff, sizeof(buff), 0, (sockaddr *)&servAddr_, sizeof(servAddr_));
        DEBUG("SENT! %d", len);
    }

private:
    struct sockaddr_in servAddr_;
    FileDescriptor socket_;

};

} /* Net */

#endif /* TESTUDP_h */
