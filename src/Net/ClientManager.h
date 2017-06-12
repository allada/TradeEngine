#ifndef ClientManager_h
#define ClientManager_h

#include "../Threading/SocketTasker.h"
#include "../Common.h"
#include "Client.h"
#include <unordered_map>

namespace Net {

struct ClientManager {
public:
  static void addClient(std::unique_ptr<Client>);

  static void removeClient(FileDescriptor);

  // Non owner reference.
  static Client* getClient(unsigned long ip);

  static size_t clientCount() { return clients_.size(); }

private:
  static std::unordered_map<unsigned long, std::unique_ptr<Client>> clients_;

};

} /* Net */

#endif /* ClientManager_h */
