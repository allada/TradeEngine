#ifndef UDPSocketRecvTask_h
#define UDPSocketRecvTask_h

#include "../Threading/SocketTasker.h"
#include <netinet/in.h>
#include <unordered_map>
#include "../API/StreamDispatcher.h"
#include "../Common.h"

// TODO Move this config data somewhere else.
#define SERV_PORT 3010
#define MAX_BUFF_SIZE 65507

namespace Net {

class UDPSocketRecvTask : public Threading::SocketTasker {
public:
    UDPSocketRecvTask()
        : UDPSocketRecvTask(WrapUnique(new API::StreamDispatcher())) { }
    explicit UDPSocketRecvTask(std::unique_ptr<API::StreamDispatcher>);

    void run() override;

    static size_t total_data_received_;

private:
    std::unique_ptr<API::StreamDispatcher> stream_dispatcher_;

};

} /* Net */

#endif /* UDPSocketRecvTask_h */
