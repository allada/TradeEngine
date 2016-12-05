#include "UDPSocketRecvTask.h"

#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

using namespace Net;

UDPSocketRecvTask::UDPSocketRecvTask()
    : Thread::SocketTasker(static_cast<FileDescriptor>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
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

void UDPSocketRecvTask::run()
{
    unsigned char buff[MAX_BUFF_SIZE];
    struct sockaddr_in remoteAddr;
    socklen_t addrLen = sizeof(remoteAddr);
    
    size_t len = recvfrom(socket_, &buff, MAX_BUFF_SIZE, MSG_DONTWAIT, (struct sockaddr *) &remoteAddr, &addrLen);
    size_t addr_hash = hashAddr(remoteAddr);

    std::unique_ptr<APIDataPackage> package;
    if (partial_packages_.count(addr_hash)) {
        package = std::move(partial_packages_[addr_hash]);
    } else {
        package = WrapUnique(new APIDataPackage);
    }
    size_t consumedLength = package->appendData(buff[0], len);

    EXPECT_TRUE(package->done() || consumedLength == len); // API Data Package has not consumed all the data and is not done

    if (package->done()) {
        // TODO Send to IO thread?
        this->dataReceived(std::move(package));
    }
    DEBUG("Socket %d got %d bytes of data", socket_, len);
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        DEBUG("Socket %d got EAGAIN");
    }
}

void UDPSocketRecvTask::dataReceived(std::unique_ptr<APIDataPackage> package)
{
    
}

size_t UDPSocketRecvTask::hashAddr(const sockaddr_in& addr)
{
    return std::hash<unsigned long>()(addr.sin_addr.s_addr) ^ std::hash<unsigned short>()(addr.sin_port);
}
