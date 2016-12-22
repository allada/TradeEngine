#ifndef UDPSocketRecvTask_h
#define UDPSocketRecvTask_h

#include "../Threading/SocketTasker.h"
#include <netinet/in.h>
#include <unordered_map>
#include "../API/StreamDispatcher.h"
#include "../Common.h"

// TODO Move this config data somewhere else.
#define SERV_PORT 3010
#define MAX_BUFF_SIZE 65600

namespace Net {

class UDPSocketRecvTask : public Threading::SocketTasker {
public:
    UDPSocketRecvTask();

    void run() override;

private:
    API::StreamDispatcher stream_dispatcher_;

};

} /* Net */

#endif /* UDPSocketRecvTask_h */
