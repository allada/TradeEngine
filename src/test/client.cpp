#include "gtest/gtest.h"

#include "Net/Client.h"
#include "Net/ClientManager.h"
#include <sys/socket.h>

namespace {
using namespace Net;

constexpr uint8_t VERSION = 0;

// This cannot be changed for reverse compatibility reasons.
enum Action : uint8_t {
  CLOSE  = 0,
  CLOSED = 1,
  PING   = 2,
  PONG   = 3,
  ACK    = 4,
  NACK   = 5,
};

class ClientTest : public ::testing::Test {
    void SetUp()
    {
        FileDescriptor sockets[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
            EXPECT_TRUE(false);
        }
        endPointSock_ = sockets[0];
        serviceSock_ = sockets[1];
    }

    void TearDown()
    {
        close(endPointSock_);
        close(serviceSock_);
    }

protected:
    bool isSocketClosed(FileDescriptor fd)
    {
        unsigned char data;
        return ::send(fd, &data, 1, MSG_NOSIGNAL) == -1;
    }

    FileDescriptor endPointSock_;
    FileDescriptor serviceSock_;

};

TEST_F(ClientTest, ClientPongRespondedOnPing) {
  Client client(serviceSock_, 0);
  unsigned char data[16] = { VERSION, PING };
  ::send(endPointSock_, data, 2, 0);
  client.run();
  ssize_t len = ::recv(endPointSock_, data, 16, 0);
  EXPECT_EQ(len, 2);
  EXPECT_EQ(data[0], VERSION);
  EXPECT_EQ(data[1], PONG);
}

TEST_F(ClientTest, ClientDestructionSendsClose) {
  {
    Client client(serviceSock_, 0);
  }

  unsigned char data[16];
  ssize_t len = ::recv(endPointSock_, data, 16, 0);
  EXPECT_EQ(len, 2);
  EXPECT_EQ(data[0], VERSION);
  EXPECT_EQ(data[1], CLOSED);

  EXPECT_TRUE(isSocketClosed(endPointSock_));
}

TEST_F(ClientTest, ClientClosesConnectOnCloseCommand) {
  ClientManager::addClient(WrapUnique(new Client(serviceSock_, 0)));

  unsigned char data[16] = { VERSION, CLOSE };
  ::send(endPointSock_, data, 2, 0);

  Client* client = ClientManager::getClient(0);
  EXPECT_NE(client, nullptr);
  EXPECT_EQ(ClientManager::clientCount(), 1);
  client->run();
  EXPECT_EQ(ClientManager::clientCount(), 0);

  size_t len = ::recv(endPointSock_, data, 16, 0);
  EXPECT_EQ(len, 2);
  EXPECT_EQ(data[0], VERSION);
  EXPECT_EQ(data[1], CLOSED);

  EXPECT_TRUE(isSocketClosed(endPointSock_));
}

} // namespace
