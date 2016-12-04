#ifndef UDPSocketRecvTask_h
#define UDPSocketRecvTask_h

#include "../Thread/SocketTasker.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <unordered_map>
#include "APIDataPackage.h"

// TODO Move this config data somewhere else.
#define SERV_PORT 3010
#define MAX_BUFF_SIZE 2048

namespace Net {

class UDPSocketRecvTask : public Thread::SocketTasker {
public:
    UDPSocketRecvTask()
        : Thread::SocketTasker(static_cast<FileDescriptor>(socket(AF_INET, SOCK_DGRAM, 0)))
    {
        struct sockaddr_in servAddr;
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port = htons(SERV_PORT);

        bind(socket_, (struct sockaddr *) &servAddr, sizeof(servAddr));
        fcntl(socket_, F_SETFL, O_NONBLOCK);
        //listen(socket_, 0);

        DEBUG("UDP Socket %d Sniffing", socket_);
    }

    void run() override
    {
        unsigned char buff[MAX_BUFF_SIZE];
        struct sockaddr_in remoteAddr;
        socklen_t addrLen = sizeof(remoteAddr);
        //bool firstLoop = true;
        RERUN_RECV: {
            size_t len = recvfrom(socket_, &buff, MAX_BUFF_SIZE, MSG_DONTWAIT, (struct sockaddr *) &remoteAddr, &addrLen);
            size_t addr_hash = hashAddr(remoteAddr);

            if (partial_packages_.count(addr_hash)) {
                std::unique_ptr<APIDataPackage> package = std::move(partial_packages_[addr_hash]);
                size_t consumedLength = package->appendData(&buff[0], len);
                if (package->done()) {
                    // TODO Send to IO thread?

                }
                EXPECT_TRUE(package->done() || consumedLength == len); // API Data Package has not consumed all the data and is not done
            }
            DEBUG("Socket %d got %d bytes of data", socket_, len);
            if (errno == EAGAIN) {
                DEBUG("Socket %d got EAGAIN");
                //firstLoop = false;
                goto RERUN_RECV;
            }
        }
    }

private:
    size_t hashAddr(const sockaddr_in& addr)
    {
        return std::hash<unsigned long>()(addr.sin_addr.s_addr) ^ std::hash<unsigned short>()(addr.sin_port);
    }

    std::unordered_map<size_t, std::unique_ptr<APIDataPackage>> partial_packages_;

};

} /* Net */

#endif /* UDPSocketRecvTask_h */
