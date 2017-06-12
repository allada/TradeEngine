#include "ClientManager.h"

// #include <sys/socket.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <netinet/in.h>

using namespace Net;

std::unordered_map<unsigned long, std::unique_ptr<Client>> ClientManager::clients_;

void ClientManager::addClient(std::unique_ptr<Client> client) {
  clients_.erase(client->ip());
  clients_.emplace(client->ip(), std::move(client));
}

void ClientManager::removeClient(FileDescriptor fd) {
  clients_.erase(fd);
}

Client* ClientManager::getClient(unsigned long ip) {
  auto it = clients_.find(ip);
  if (it == clients_.end()) {
    return nullptr;
  }
  return it->second.get();
}