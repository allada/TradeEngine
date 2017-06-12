#include "Client.h"

#include "../Threading/SocketTasker.h"
#include "ClientManager.h"

using namespace Net;

constexpr int MAX_BUFF_SIZE = 65507;

enum class ProtocolPositions {
  PROTOCOL_VERSION = 0, // +1
  ACTION           = 1, // +1
  SEQ_INCREMENTER  = 2, // +4
};

size_t Client::Package::setData(unsigned char* data, size_t size)
{
  if (size < 2) {
    return 0;
  }
  data_.protocolVersion = reinterpret_cast<const uint8_t&>(data[static_cast<int>(ProtocolPositions::PROTOCOL_VERSION)]);
  data_.action = reinterpret_cast<const Action&>(data[static_cast<int>(ProtocolPositions::ACTION)]);
  if (data_.action != Action::ACK && data_.action != Action::NACK) {
    return 2;
  }
  if (size < 6) {
    return -1;
  }
  data_.seqInc = reinterpret_cast<const uint32_t&>(data[static_cast<int>(ProtocolPositions::SEQ_INCREMENTER)]);
  return 6;
}

size_t Client::Package::size() const
{
  switch (data_.action) {
    case Action::CLOSE:
    case Action::CLOSED:
    case Action::PING:
    case Action::PONG:
      return 2;
    case Action::ACK:
    case Action::NACK:
      return sizeof(data_);
  }
}

void Client::send(const Package& package) {
  EXPECT_FALSE(closed_);
  // TODO This needs to handle cases where not all the data sent better.
  ::send(socket_, package.data(), package.size(), 0);
}

void Client::close()
{
  if (closed_) {
    return;
  }
  send(Package(Package::Action::CLOSED));
  closed_ = true;
  Threading::SocketTasker::close();
  ClientManager::removeClient(ip_);
}

void Client::run() {
  // TODO Maybe fast allocator here?
  std::vector<unsigned char> buff(MAX_BUFF_SIZE);
  for (;;) {
    ssize_t len = recv(socket_, buff.data(), MAX_BUFF_SIZE, MSG_DONTWAIT);
    if (len == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
        // TODO Better error checking.
        DEBUG("Socket %d got error: %d", socket_, errno);
    }

    ssize_t consumedLen = 0;
    while (len > consumedLen) {
      Package package;
      size_t packageDataSize = package.setData(buff.data() + consumedLen, len - consumedLen);
      if (packageDataSize <= 0) {
        WARNING("Client %d sent data that could not be turned into a package.", socket_);
        break;
      }
      consumedLen += packageDataSize;

      switch (package.action()) {
        case Package::Action::CLOSE:
          this->close();
          return;
        case Package::Action::PING:
          send(Package(Package::Action::PONG));
          break;
        case Package::Action::PONG:
          // No need to do anything here. We should have updated it's timeout already.
          break;
        case Package::Action::CLOSED:
          WARNING("Client %d sent CLOSED, but clients should not send CLOSED.", socket_);
          return;
        case Package::Action::ACK:
          WARNING("Client %d sent ACK, but clients should not send ACK.", socket_);
          return;
        case Package::Action::NACK:
          WARNING("Client %d sent NACK, but clients should not send NACK.", socket_);
          return;
      }
    }
  }
}
