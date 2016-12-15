#include <csignal>
#include <string.h>
#include "Thread/SocketPollThread.h"
#include "Thread/TaskQueueThread.h"
#include "Thread/ThreadManager.h"
#include "Thread/MainThread.h"
#include "Common.h"

#include "Net/UDPSocketRecvTask.h"
#include <arpa/inet.h>
#include <errno.h>
#include <chrono>
#include "Engine/Order.h"
#include "Engine/BuyLedger.h"
#include "Engine/SellLedger.h"
#include "includes/crc32.h"

std::shared_ptr<Thread::MainThread> mainThread;
typedef std::chrono::high_resolution_clock Clock;

std::string coin_name;

void signalHandler(int signal)
{
    EXPECT_MAIN_THREAD();
    DEBUG("Got termination request");
    Thread::ThreadManager::killAll();
}

class UDPStreamHelper {
public:
    UDPStreamHelper(std::string ip, int port)
        : socket_(static_cast<FileDescriptor>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
    {
        servAddr_.sin_family = AF_INET;
        servAddr_.sin_addr.s_addr = inet_addr(ip.c_str());
        servAddr_.sin_port = htons(port);

        const int broadcast = 1;
        setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
        DEBUG("UDP Helper %d ready", socket_);
    }

    template <typename T>
    void send(const T& data)
    {
        auto len = sendto(socket_, data.data(), data.size(), 0, (sockaddr *) &servAddr_, sizeof(servAddr_));
        EXPECT_EQ(len, data.size());
    }

private:
    struct sockaddr_in servAddr_;
    FileDescriptor socket_;

};

// TODO I dont like this.
extern std::shared_ptr<Thread::TaskQueueThread> uiThread;

std::array<unsigned char, 38> packCreateOrder(uint64_t price, uint64_t qty, Engine::Order::OrderType type)
{
    std::array<unsigned char, 38> out;
    out[0] = 1;
    out[5] = type == Engine::Order::OrderType::BUY ? ACTION_CREATE_BUY_ORDER : ACTION_CREATE_SELL_ORDER;
    uint64_t orderId;
    memcpy(&out[6], &orderId, sizeof(uint64_t));
    memcpy(&out[14], &price, sizeof(uint64_t));
    memcpy(&out[22], &qty, sizeof(uint64_t));
    memset(&out[30], '\0', sizeof(uint64_t));
    uint32_t checksum = ::crc32(&out[5], 33);
    DBG("checksum: %x", checksum);
    memcpy(&out[1], &checksum, sizeof(uint32_t));
    return out;
}

int main(int argc, char* argv[])
{
    mainThread = std::make_shared<Thread::MainThread>();
    Thread::ThreadManager::setMainThread(mainThread);
    {
        uiThread =
                Thread::createThread<Thread::TaskQueueThread>("UI");
        std::shared_ptr<Thread::SocketPollThread> ioThread =
                Thread::createThread<Thread::SocketPollThread>("IO");

        ioThread->addSocketTasker(WrapUnique(new Net::UDPSocketRecvTask));

        UDPStreamHelper udpStreamHelper("127.0.0.5", SERV_PORT);


        std::vector<unsigned char> package;
        udpStreamHelper.send(package);


    const auto NUM_ORDERS = 2500000;
    const auto start = Clock::now();
    size_t trades = 0;
    udpStreamHelper.send(packCreateOrder(40, 107, Engine::Order::OrderType::SELL));
    udpStreamHelper.send(packCreateOrder(40, 107, Engine::Order::OrderType::SELL));
    // for (auto i = NUM_ORDERS; i >= 0; --i) {
    //     //udpStreamHelper.send(packCreateOrder(40, 107, Engine::Order::OrderType::SELL));
    //     packCreateOrder(9, 8, Engine::Order::OrderType::BUY);
    //     packCreateOrder(2, 5, Engine::Order::OrderType::SELL);
    //     packCreateOrder(1, 6, Engine::Order::OrderType::BUY);
    //     packCreateOrder(6, 9, Engine::Order::OrderType::SELL);
    //     packCreateOrder(5, 10, Engine::Order::OrderType::BUY);
    //     packCreateOrder(8, 7, Engine::Order::OrderType::SELL);
    //     packCreateOrder(7, 8, Engine::Order::OrderType::BUY);
    //     packCreateOrder(2, 5, Engine::Order::OrderType::SELL);
    //     packCreateOrder(1, 6, Engine::Order::OrderType::BUY);
    //     packCreateOrder(4, 3, Engine::Order::OrderType::SELL);
    //     packCreateOrder(3, 4, Engine::Order::OrderType::BUY);
    // }
    const auto end = Clock::now();
    const auto timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000 / 1000;
    fprintf(stderr, "Orders Executed: %lu\n", trades);
    fprintf(stderr, "Time taken: %ld milli seconds\n", timeTaken);
    //fprintf(stderr, "Orders per second: %lu\n", trades / (timeTaken / 1000));




        signal(SIGINT, signalHandler);
        if (argc < 2 || strlen(argv[1]) < 1) {
            fprintf(stderr, "First parameter must be a valid coin abbr name.\n");
            raise(SIGINT);
        }
        coin_name = argv[1];
        Thread::ThreadManager::joinAll();
    }
    DEBUG("Exiting main function");
    return 0;
}
