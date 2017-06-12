#include "gtest/gtest.h"

#include <arpa/inet.h>
#include "API/DataPackage.h"
#include "Net/UDPSocketRecvTask.h"
#include <errno.h>
#include <algorithm>

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

std::vector<unsigned char> receivedData_;

class HandlePackageForTest : public API::StreamDispatcher {
public:
    void processData(Hash originHash, unsigned char* begin, size_t len) override
    {
        receivedData_.reserve(len);
        std::copy(begin, begin + len, std::back_inserter(receivedData_));
    }
};

class UDPTest : public ::testing::Test {
};

TEST(UDPTest, RecvBasicData) {
    UDPSocketRecvTask udpRecv(WrapUnique(new HandlePackageForTest));
    UDPStreamHelper udpStreamHelper(UDP_TEST_IP, Net::UDPSocketRecvTask::SERV_PORT);

    const std::string rawData1 = "Hello foo bar!\n";
    const std::vector<unsigned char> dummyData1(rawData1.begin(), rawData1.end());

    udpStreamHelper.send(dummyData1);
    udpRecv.run();

    ASSERT_EQ(receivedData_.size(), dummyData1.size());
    ASSERT_EQ(receivedData_, dummyData1);
    receivedData_.clear();
}

TEST(UDPTest, RecvMultipleChunksOfData) {
    UDPSocketRecvTask udpRecv(WrapUnique(new HandlePackageForTest));
    UDPStreamHelper udpStreamHelper(UDP_TEST_IP, Net::UDPSocketRecvTask::SERV_PORT);

    const std::string rawData1 = "foo\n";
    const std::vector<unsigned char> dummyData1(rawData1.begin(), rawData1.end());
    const std::string rawData2 = "baring";
    const std::vector<unsigned char> dummyData2(rawData2.begin(), rawData2.end());

    udpStreamHelper.send(dummyData1);
    udpStreamHelper.send(dummyData2);
    udpRecv.run();

    ASSERT_EQ(receivedData_.size(), dummyData1.size() + dummyData2.size());

    std::vector<unsigned char> receivedDataSlice1(receivedData_.begin(), receivedData_.begin() + dummyData1.size());
    std::vector<unsigned char> receivedDataSlice2(receivedData_.begin() + dummyData1.size(), receivedData_.end());
    ASSERT_EQ(receivedDataSlice1, dummyData1);
    ASSERT_EQ(receivedDataSlice2, dummyData2);
    receivedData_.clear();
}

} // namespace
