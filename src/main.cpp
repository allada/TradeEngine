#include <csignal>
#include <string.h>
#include "Threading/SocketPollThread.h"
#include "Threading/TaskQueueThread.h"
#include "Threading/ThreadManager.h"
#include "Threading/MainThread.h"
#include "Common.h"

#include "Net/UDPSocketRecvTask.h"
#include <arpa/inet.h>
#include <errno.h>
#include <chrono>
#include "Engine/Order.h"
#include "Engine/BuyLedger.h"
#include "Threading/TaskQueue.h"
#include "Engine/SellLedger.h"
#include "includes/crc32.h"
#include <endian.h>
#include "Engine/Trade.h"
#include <fcntl.h>
#include "Threading/SocketTasker.h"
#include <errno.h>
#include <fstream>
#include <iostream>
#include "Engine/ProcessOrderTask.h"

std::shared_ptr<Threading::MainThread> mainThread;
typedef std::chrono::high_resolution_clock Clock;

std::string coin_name;

void signalHandler(int signal)
{
    EXPECT_MAIN_THREAD();
    DEBUG("Got termination request");
    Threading::ThreadManager::killAll();
}

uint64_t totalDataSent = 0;

#define MAX_PACKET_SIZE 65507lu

struct sockaddr_in servAddr_;
FileDescriptor udpSendSock;
// TODO I dont like this.
std::shared_ptr<Threading::TaskQueueThread> uiThread;
std::shared_ptr<Threading::SocketPollThread> ioThread;

std::shared_ptr<Threading::TaskQueueThread> ioSendThread;
std::shared_ptr<Threading::TaskQueueThread> ioSendNetworkThread;

class TaskSendBuffer : public virtual Threading::Tasker {
public:
    void run() override
    {
        std::vector<unsigned char> chunk(MAX_PACKET_SIZE);
        for (;;) {
            size_t bufferSize = TaskSendBuffer::dataQueue.countAsReader();
            if (!bufferSize)
                break;
            size_t chunkSize = std::min(bufferSize, MAX_PACKET_SIZE);
            EXPECT_GT(MAX_PACKET_SIZE + 1, chunkSize);
            
            // DBG("%lu", chunk.size());
            TaskSendBuffer::dataQueue.popChunk(chunk, chunkSize);

            for (size_t i = 0; i < chunkSize; ) {
                //std::this_thread::sleep_for(std::chrono::milliseconds(50));
                auto len = sendto(udpSendSock, chunk.data() + i, chunkSize - i, 0, (struct sockaddr *) &servAddr_, sizeof(servAddr_));
                if (len == -1) {
                   fprintf(stderr, "ERROR! %d\n", errno);
                }
                //fprintf(stderr, "Sent %lu bytes\n", len);
                if (len == -1) {
                    fprintf(stderr, "ERROR! %d\n", errno);
                } else {
                    i += len;
                    totalDataSent += len;
                }
            }
        }
        isRunning.clear(std::memory_order_release);
    }

    static inline void scheduleForRun()
    {
        if (isRunning.test_and_set(std::memory_order_acquire) == 0) {
            ioSendNetworkThread->addTask(WrapUnique(new TaskSendBuffer));
        }
    }

    static std::atomic_flag isRunning;
    static Threading::TaskQueue<unsigned char, 16777215> dataQueue;
};

Threading::TaskQueue<unsigned char, 16777215> TaskSendBuffer::dataQueue;
std::atomic_flag TaskSendBuffer::isRunning = ATOMIC_FLAG_INIT;


class CreateOrderTasker : public virtual Threading::Tasker {
    FAST_ALLOCATE(CreateOrderTasker)
public:
    CreateOrderTasker(FileDescriptor socket, std::unique_ptr<API::DataPackage> package)
        // : Threading::SocketTasker(static_cast<FileDescriptor>(eventfd(0, EFD_SEMAPHORE | O_NONBLOCK)))
        : package_(std::move(package))
        , socket_(socket) { }

    void run() override
    {
        // auto count = queue_.countAsReader();
        // std::vector<unsigned char> sendData;
        // for (auto i = count; i > 0; --i) {
            // std::unique_ptr<API::DataPackage> package = queue_.pop();
            //sendData.insert(sendData.end(), package->data().cbegin(), package->data().cend());
        // }
        //CreateOrderTasker::network_send_buffer_->queueData(std::move(package_));
        //size_t len = package_->data().size();
        //size_t len = sendto(socket_, package_.data().data(), package_.data().size(), 0, (struct sockaddr *) &servAddr_, sizeof(servAddr_));
        // if (len == -1) {
        //     fprintf(stderr, "ERROR! %d\n", errno);
        // }
        // if (len == EAGAIN)
        //     fprintf(stderr, "EAGAIN!\n");
        // else if (len == EWOULDBLOCK)
        //     fprintf(stderr, "EWOULDBLOCK\n");
        // totalDataSent += len + 20;

        TaskSendBuffer::dataQueue.pushChunk(package_->data());
        TaskSendBuffer::scheduleForRun();
    }

private:
    std::unique_ptr<API::DataPackage> package_;
    FileDescriptor socket_;

};


inline void runOrder(const Engine::Order& order)
{
    //PROFILE_START()
    //PROFILE_END()
    // API::DataPackage package(order);
    // TaskSendBuffer::dataQueue.pushChunk(package.data());
    // TaskSendBuffer::scheduleForRun();
    //ioSendThread->addTask(WrapUnique(new CreateOrderTasker(udpSendSock, WrapUnique(new API::DataPackage(order)))));
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
    mainThread = std::make_shared<Threading::MainThread>();
    Threading::ThreadManager::setMainThread(mainThread);
    {
        uiThread =
                Threading::createThread<Threading::TaskQueueThread>("UI");
        ioSendThread =
                Threading::createThread<Threading::TaskQueueThread>("IO Send");
        ioSendNetworkThread =
                Threading::createThread<Threading::TaskQueueThread>("IO NetSend");
        ioThread =
                Threading::createThread<Threading::SocketPollThread>("IO");

        ioThread->addSocketTasker(WrapUnique(new Net::UDPSocketRecvTask));

        udpSendSock = static_cast<FileDescriptor>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

        servAddr_.sin_family = AF_INET;
        servAddr_.sin_addr.s_addr = inet_addr("127.0.0.5");
        servAddr_.sin_port = htons(SERV_PORT);

        const int broadcast = 1;
        setsockopt(udpSendSock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(const int));
        //fcntl(udpSendSock, F_SETFL, O_NONBLOCK);

        const auto NUM_ORDERS = 1000000; // 5000000;
        const auto start = Clock::now();

        Engine::Trade::addDeligate(WrapUnique(new SpeedTradeDeligate));
        std::ifstream orderStream;
        orderStream.open("orders.log", std::ios::in | std::ios::binary | std::ios::ate);
        std::streampos size = orderStream.tellg();
        orderStream.seekg(0, std::ios::beg);

std::unique_ptr<API::DataPackage> partialPackage;

        std::vector<unsigned char> orderData(40);
        for (size_t i = size; i > 0;) {
            size_t sz = i > 40 ? 40 : i;
            
            orderStream.read(reinterpret_cast<char *>(orderData.data()), sz);
            //TaskSendBuffer::dataQueue.pushChunk(orderData, sz);
            //TaskSendBuffer::scheduleForRun();


size_t consumedLength = 0;
            while (consumedLength < sz) {

                std::unique_ptr<API::DataPackage> package;
                if (partialPackage) {
                    package = std::move(partialPackage);
                } else {
                    package = WrapUnique(new API::DataPackage);
                }
                consumedLength += package->appendData(&orderData[consumedLength], sz - consumedLength);

                EXPECT_TRUE(package->isDone() || consumedLength == sz); // API Data Package has not consumed all the data and is not done

                if (package->isDone()) {
                    // TODO Send to IO thread?
                    if (UNLIKELY(!package->quickVerify())) {
                        goto LEAVE;
                        continue;
                    }
                    uiThread->addTask(WrapUnique(new Engine::ProcessOrderTask(std::move(package))));
                } else {
                    partialPackage = std::move(package);
                }


            }



            i -= sz;
        }
        LEAVE:
        orderStream.close();

        // const auto NUM_ORDERS = 1000000; // 5000000;
        // const auto start = Clock::now();
        // for (auto i = 0; i < NUM_ORDERS; i++) {
        //     if (i % 100000 == 1)
        //         fprintf(stderr, "\r%.1f%% done", ((i + 1) / float(NUM_ORDERS)) * 100);
        //     runOrder(Engine::Order(2, 5, Engine::Order::OrderType::SELL));
        //     runOrder(Engine::Order(1, 6, Engine::Order::OrderType::BUY));
        //     runOrder(Engine::Order(6, 9, Engine::Order::OrderType::SELL));
        //     runOrder(Engine::Order(5, 10, Engine::Order::OrderType::BUY));
        //     runOrder(Engine::Order(8, 7, Engine::Order::OrderType::SELL));
        //     runOrder(Engine::Order(7, 8, Engine::Order::OrderType::BUY));
        //     runOrder(Engine::Order(2, 5, Engine::Order::OrderType::SELL));
        //     runOrder(Engine::Order(1, 6, Engine::Order::OrderType::BUY));
        //     runOrder(Engine::Order(4, 3, Engine::Order::OrderType::SELL));
        //     runOrder(Engine::Order(3, 4, Engine::Order::OrderType::BUY));
        //     // if (i % 10 == 0)
        //     //     write(sock, &Threading::incrementer_, sizeof(Threading::incrementer_));
        // }


        const auto end = Clock::now();
        const auto timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000 / 1000;

std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        const auto ordersExecuted = Engine::SellLedger::processedCount() + Engine::BuyLedger::processedCount();


        fprintf(stderr, "Trades Executed: %lu\n", trades);
        fprintf(stderr, "Orders Executed: %lu\n", ordersExecuted);
        fprintf(stderr, "Time taken: %ld milli seconds\n", timeTaken);
        if (timeTaken / 1000 > 0)
            fprintf(stderr, "Trades per second: %lu\n", trades / (timeTaken / 1000));
        fprintf(stderr, "Data Sent: %.2f mb\n", float(totalDataSent) / 1000 / 1000);
        fprintf(stderr, "Orders Per Second: %.1f\n", float(timeTaken) / ordersExecuted);
        if ((timeTaken / 1000) / 1000 / 1000 > 0)
            fprintf(stderr, "Bytes Per Second: %.1f mbps\n", float(totalDataSent) / (timeTaken / 1000) / 1000 / 1000);
        fprintf(stderr, "ACCUM_TIME: %lu ms\n", PROFILER_ACCUM / 1000 / 1000);

        //signal(SIGINT, signalHandler);
        if (argc < 2 || strlen(argv[1]) < 1) {
            fprintf(stderr, "First parameter must be a valid coin abbr name.\n");
            raise(SIGINT);
        }
        coin_name = argv[1];
        Threading::ThreadManager::killAll();
        Threading::ThreadManager::joinAll();
        // MemoryBucket::terminate();

        // Engine::SellLedger::reset();
        // Engine::BuyLedger::reset();
    }
    DEBUG("Exiting main function");
    return 0;
}
