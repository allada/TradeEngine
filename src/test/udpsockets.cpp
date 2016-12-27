#include "gtest/gtest.h"

#include <arpa/inet.h>
#include "API/DataPackage.h"
#include "Net/UDPSocketRecvTask.h"
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
        setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(const int));
        DEBUG("UDP Helper %d ready", socket_);
    }

    template <typename T>
    void send(T data)
    {
        auto len = sendto(socket_, data.data(), data.size(), 0, (const sockaddr *) &servAddr_, sizeof(struct sockaddr));
        EXPECT_EQ(len, data.size());
        EXPECT_NE(errno, 0) << strerror(errno);
    }

private:
    struct sockaddr_in servAddr_;
    FileDescriptor socket_;

};

std::unique_ptr<API::DataPackage> package_;

class HandlePackageForTest : public API::StreamDispatcher {
public:
    void processPackage(std::unique_ptr<API::DataPackage> package) override
    {
        package_ = std::move(package);
    }
};

class UDPTest : public ::testing::Test {
};

TEST(UDPTest, Recv1Byte) {
    UDPSocketRecvTask udpRecv(WrapUnique(new HandlePackageForTest));
    UDPStreamHelper udpStreamHelper(UDP_TEST_IP, SERV_PORT);

    const std::array<unsigned char, 38> dummyData1 = {"Hello foo bar!\n"};

    udpStreamHelper.send(dummyData1);
    udpRecv.run();

    ASSERT_NE(package_, nullptr);

    ASSERT_EQ(package_->data().size(), dummyData1.size());
    ASSERT_EQ(package_->data(), dummyData1);
    package_ = nullptr;
}

} // namespace
