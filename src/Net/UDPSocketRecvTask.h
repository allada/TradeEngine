#ifndef UDPSocketRecvTask_h
#define UDPSocketRecvTask_h

#include "../Thread/SocketTasker.h"
#include <netinet/in.h>
#include <unordered_map>
#include "APIDataPackage.h"
#include "../Common.h"

// TODO Move this config data somewhere else.
#define SERV_PORT 3010
#define MAX_BUFF_SIZE 2048

namespace Net {

class UDPSocketRecvTask : public Thread::SocketTasker {
public:
    UDPSocketRecvTask();

    void run() override;

    VIRTUAL_FOR_TEST void packageReady(std::unique_ptr<APIDataPackage>);

private:
    size_t hashAddr(const sockaddr_in&);

    std::unordered_map<size_t, std::unique_ptr<APIDataPackage>> partial_packages_;

};

} /* Net */

#endif /* UDPSocketRecvTask_h */
