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
#include <endian.h>
#include "Engine/Trade.h"

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
    void send(const T& data) const
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

inline void runOrder(const Engine::Order& order, const UDPStreamHelper& udpStreamHelper) {
    Net::APIDataPackage package(order);
    udpStreamHelper.send(package.data());
}

size_t trades = 0;

class SpeedTradeDeligate : public virtual Engine::TradeDeligate {
    void tradeExecuted(std::shared_ptr<Engine::Trade>) const
    {
        ++trades;
    }
};

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

        Engine::Trade::addDeligate(WrapUnique(new SpeedTradeDeligate));

        const auto NUM_ORDERS = 2500000;
        const auto start = Clock::now();
        for (auto i = NUM_ORDERS; i >= 0; --i) {
            runOrder(Engine::Order(2, 5, Engine::Order::OrderType::SELL), udpStreamHelper);
            runOrder(Engine::Order(1, 6, Engine::Order::OrderType::BUY), udpStreamHelper);
            runOrder(Engine::Order(6, 9, Engine::Order::OrderType::SELL), udpStreamHelper);
            runOrder(Engine::Order(5, 10, Engine::Order::OrderType::BUY), udpStreamHelper);
            runOrder(Engine::Order(8, 7, Engine::Order::OrderType::SELL), udpStreamHelper);
            runOrder(Engine::Order(7, 8, Engine::Order::OrderType::BUY), udpStreamHelper);
            runOrder(Engine::Order(2, 5, Engine::Order::OrderType::SELL), udpStreamHelper);
            runOrder(Engine::Order(1, 6, Engine::Order::OrderType::BUY), udpStreamHelper);
            runOrder(Engine::Order(4, 3, Engine::Order::OrderType::SELL), udpStreamHelper);
            runOrder(Engine::Order(3, 4, Engine::Order::OrderType::BUY), udpStreamHelper);
        }

        const auto end = Clock::now();
        const auto timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000 / 1000;
        fprintf(stderr, "Orders Executed: %lu\n", trades);
        fprintf(stderr, "Time taken: %ld milli seconds\n", timeTaken);
        fprintf(stderr, "Orders per second: %lu\n", trades / (timeTaken / 1000));




        Thread::ThreadManager::killAll();
        //signal(SIGINT, signalHandler);
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
