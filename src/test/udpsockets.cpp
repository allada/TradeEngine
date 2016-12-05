#include "gtest/gtest.h"

#include <csignal>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>

#include <arpa/inet.h>

#include "Net/APIDataPackage.h"
#include "Net/udpSocketRecvTask.h"

#include <chrono>
#include <unistd.h>
#include <errno.h>

#define UDP_TEST_IP "127.0.0.5"

namespace {
using namespace Net;

class UDPStreamHelper {
public:
    UDPStreamHelper(std::string ip, int port)
        : socket_(static_cast<FileDescriptor>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
    {
        EXPECT_GT(socket_, 0) << "Opening datagram socket error";

        servAddr_.sin_family = AF_INET;
        servAddr_.sin_addr.s_addr = inet_addr(ip.c_str());
        servAddr_.sin_port = htons(port);

        const int broadcast = 1;
        setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
        DEBUG("UDP Helper %d ready", socket_);
    }

    void send(const std::vector<unsigned char>& data)
    {
        auto len = sendto(socket_, data.data(), data.size(), 0, (sockaddr *) &servAddr_, sizeof(servAddr_));
        EXPECT_EQ(len, data.size());
        EXPECT_NE(errno, 0) << strerror(errno);
    }

private:
    struct sockaddr_in servAddr_;
    FileDescriptor socket_;

};

class UDPSocketRecvTaskMock : public UDPSocketRecvTask {
public:
    void dataReceived(std::unique_ptr<APIDataPackage> package) override
    {
        package_ = std::move(package);
    }

    std::unique_ptr<APIDataPackage> packageForTest() { return std::move(package_); }

private:
    std::unique_ptr<APIDataPackage> package_;

};

class UDPTest : public ::testing::Test {
};

TEST(UDPTest, Recv1Byte) {
    UDPSocketRecvTaskMock udpSocketRecvTaskMock;
    UDPStreamHelper udpStreamHelper(UDP_TEST_IP, SERV_PORT);

    std::vector<unsigned char> dummyData1 = {'H', 'e', 'l', 'l', 'o', ' ', 'f', 'o', 'o', ' ', 'b', 'a', 'r'};

    udpStreamHelper.send(dummyData1);
    udpSocketRecvTaskMock.run();

    std::unique_ptr<APIDataPackage> package = udpSocketRecvTaskMock.packageForTest();

    ASSERT_EQ(package->data().size(), dummyData1.size());
    ASSERT_EQ(package->data(), dummyData1);
}

} // namespace