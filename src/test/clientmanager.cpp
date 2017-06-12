#include "gtest/gtest.h"

#include "Net/Client.h"
#include "Net/ClientManager.h"

namespace {
using namespace Net;

class ClientManagerTest : public ::testing::Test {
};

TEST(ClientManagerTest, ClientAddRemove) {
    FileDescriptor fd(42);
    unsigned long ip = 9000;
    ClientManager::addClient(WrapUnique(new Client(fd, ip)));
    Client* client = ClientManager::getClient(ip);
    EXPECT_NE(client, nullptr);
    EXPECT_EQ(client->ip(), ip);
    EXPECT_EQ(client->fileDescriptor(), fd);
    EXPECT_EQ(ClientManager::clientCount(), 1);

    ClientManager::removeClient(ip);
    client = ClientManager::getClient(ip);
    EXPECT_EQ(client, nullptr);
    EXPECT_EQ(ClientManager::clientCount(), 0);
}

TEST(ClientManagerTest, ReplaceClientOnSameFileDescriptor) {
    FileDescriptor fd(42);
    unsigned long ip = 9000;
    ClientManager::addClient(WrapUnique(new Client(fd, ip)));
    Client* client = ClientManager::getClient(ip);
    EXPECT_NE(client, nullptr);

    ClientManager::addClient(WrapUnique(new Client(FileDescriptor(12), ip)));
    Client* otherClient = ClientManager::getClient(ip);
    EXPECT_NE(client, otherClient);
    EXPECT_EQ(ClientManager::clientCount(), 1);

    ClientManager::removeClient(ip);
    EXPECT_EQ(ClientManager::clientCount(), 0);
}

TEST(ClientManagerTest, ClientDeleteSameDescriptorTwice) {
    FileDescriptor fd(42);
    unsigned long ip = 9000;
    unsigned long ip2 = 9001;
    ClientManager::addClient(WrapUnique(new Client(fd, ip)));
    ClientManager::addClient(WrapUnique(new Client(fd, ip2)));

    EXPECT_EQ(ClientManager::clientCount(), 2);
    Client* client = ClientManager::getClient(ip);
    EXPECT_NE(client, nullptr);
    client = ClientManager::getClient(ip2);
    EXPECT_NE(client, nullptr);

    ClientManager::removeClient(ip);
    EXPECT_EQ(ClientManager::clientCount(), 1);
    client = ClientManager::getClient(ip);
    EXPECT_EQ(client, nullptr);
    client = ClientManager::getClient(ip2);
    EXPECT_NE(client, nullptr);

    ClientManager::removeClient(ip);
    EXPECT_EQ(ClientManager::clientCount(), 1);
    client = ClientManager::getClient(ip);
    EXPECT_EQ(client, nullptr);
    client = ClientManager::getClient(ip2);
    EXPECT_NE(client, nullptr);

    ClientManager::removeClient(ip2);
    EXPECT_EQ(ClientManager::clientCount(), 0);
    client = ClientManager::getClient(ip);
    EXPECT_EQ(client, nullptr);
    client = ClientManager::getClient(ip2);
    EXPECT_EQ(client, nullptr);
}

TEST(ClientManagerTest, MultipleClients) {
    FileDescriptor fd(42);
    unsigned long ip = 9000;
    ClientManager::addClient(WrapUnique(new Client(fd, ip)));

    FileDescriptor fd2(43);
    unsigned long ip2 = 9001;
    ClientManager::addClient(WrapUnique(new Client(fd2, ip2)));

    Client* client1 = ClientManager::getClient(ip);
    Client* client2 = ClientManager::getClient(ip2);
    EXPECT_NE(client1, nullptr);
    EXPECT_NE(client2, nullptr);
    EXPECT_NE(client1, client2);
    EXPECT_EQ(ClientManager::clientCount(), 2);

    ClientManager::removeClient(ip);
    EXPECT_EQ(ClientManager::clientCount(), 1);
    ClientManager::removeClient(ip2);

    EXPECT_EQ(ClientManager::clientCount(), 0);
}

TEST(ClientManagerTest, MultipleRequestsForSameClientIsSamePointer) {
    FileDescriptor fd(42);
    unsigned long ip = 9000;
    ClientManager::addClient(WrapUnique(new Client(fd, ip)));

    FileDescriptor fd2(43);
    unsigned long ip2 = 9001;
    ClientManager::addClient(WrapUnique(new Client(fd2, ip2)));

    Client* client1 = ClientManager::getClient(ip);
    Client* client2 = ClientManager::getClient(ip);
    EXPECT_EQ(client1, client2);
    EXPECT_EQ(ClientManager::clientCount(), 2);

    ClientManager::removeClient(ip);
    ClientManager::removeClient(ip2);

    EXPECT_EQ(ClientManager::clientCount(), 0);
}

} // namespace
