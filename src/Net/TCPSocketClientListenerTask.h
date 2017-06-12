#ifndef TCPSocketClientListenerTask_h
#define TCPSocketClientListenerTask_h

#include "../Threading/SocketTasker.h"
#include "../Common.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>

namespace Net {

class TCPSocketClientListenerTask : public Threading::SocketTasker {
public:
  // TODO Move this config data somewhere else.
  static constexpr int SERV_PORT = 3011;
  static constexpr int MAX_BUFF_SIZE = 65507;

  TCPSocketClientListenerTask()
    : Threading::SocketTasker(static_cast<FileDescriptor>(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))
  {
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERV_PORT);

    bind(socket_, (struct sockaddr *) &servAddr, sizeof(servAddr));

    listen(sock_, 10);

    DEBUG("TCP Socket %d Sniffing", socket_);
  }

  void run() override
  {
    EXPECT_IO_THREAD();
    struct sockaddr_in clientAddr;

    int clientSock = accept(sock_, (struct sockaddr *) &clientAddr, sizeof(clientAddr));
    if (clientSock < 0) {
      WARNING("Error on accepting client socket.");
      return;
    }

    ClientManager::addClient(WrapUnique(new Client(clientSock, clientAddr)));
  }

};

} /* Net */

#endif /* TCPSocketClientListenerTask_h */
