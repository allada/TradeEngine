#include "UDPSocketRecvTask.h"

#include "../Threading/TaskQueueThread.h"
//#include "../Engine/ProcessOrderTask.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

using namespace Net;

size_t UDPSocketRecvTask::total_data_received_ = 0;

UDPSocketRecvTask::UDPSocketRecvTask(std::unique_ptr<API::StreamDispatcher> dispatcher)
    : Threading::SocketTasker(static_cast<FileDescriptor>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
    , stream_dispatcher_(std::move(dispatcher))
{
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERV_PORT);

    bind(socket_, (struct sockaddr *) &servAddr, sizeof(servAddr));
    fcntl(socket_, F_SETFL, O_NONBLOCK);

    DEBUG("UDP Socket %d Sniffing", socket_);
}

API::StreamDispatcher::Hash hashAddr(const struct sockaddr_in& addr)
{
    return API::StreamDispatcher::hash(&addr.sin_port, sizeof(addr.sin_port), API::StreamDispatcher::hash(&addr.sin_addr.s_addr, sizeof(addr.sin_addr.s_addr)));
}

void UDPSocketRecvTask::run()
{
    EXPECT_IO_THREAD();
    std::vector<unsigned char, ::FastAllocator<unsigned char>> buff(MAX_BUFF_SIZE);
    //IFVALGRIND(memset(&buff, '\0', MAX_BUFF_SIZE));
    struct sockaddr_in remoteAddr;
    IFVALGRIND(memset((char *) &remoteAddr, 0, sizeof(remoteAddr)));
    socklen_t addrLen = sizeof(remoteAddr);
    for (;;) {

        ssize_t len = recvfrom(socket_, buff.data(), MAX_BUFF_SIZE, 0, (struct sockaddr *) &remoteAddr, &addrLen);
        if (len == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            // TODO Better error checking.
            DEBUG("Socket %d got error: %d", socket_, errno);
        }
        UDPSocketRecvTask::total_data_received_ += len;
        stream_dispatcher_->processData(hashAddr(remoteAddr), buff.data(), len);
    }
}
