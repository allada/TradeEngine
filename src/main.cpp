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
#include <sys/ioctl.h>
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

#define MAX_PACKET_SIZE ((65507lu / PACKAGE_SIZE) * PACKAGE_SIZE)

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
            // We must have enough data to send.
            if (bufferSize < PACKAGE_SIZE) {
                break;
            }
            size_t chunkSize = (std::min(bufferSize, MAX_PACKET_SIZE) / PACKAGE_SIZE) * PACKAGE_SIZE;
            EXPECT_GT(MAX_PACKET_SIZE + 1, chunkSize);
            EXPECT_EQ(chunkSize % PACKAGE_SIZE, 0);
            
            TaskSendBuffer::dataQueue.popChunk(chunk, chunkSize);

            for (size_t i = 0; i < chunkSize; ) {

std::this_thread::sleep_for(std::chrono::microseconds(10));
                //std::this_thread::sleep_for(std::chrono::microseconds(700));
                //std::this_thread::sleep_for(std::chrono::microseconds(1233));
                //std::this_thread::sleep_for(std::chrono::microseconds(2033));
                //std::this_thread::sleep_for(std::chrono::microseconds(1700));

                ssize_t len = sendto(udpSendSock, chunk.data() + i, chunkSize, 0, (struct sockaddr *) &servAddr_, sizeof(servAddr_));

                if (errno) {
                    //DBG("%lu", errno);
                }

                EXPECT_EQ(chunkSize, len);
                //fprintf(stderr, "Sent %lu bytes\n", len);
                if (len == -1) {
                    fprintf(stderr, "ERROR! %d\n", errno);
                } else {
                    i += len; //(len / PACKAGE_SIZE) * PACKAGE_SIZE;
                    totalDataSent += len;
                }
            }
        }
        isRunning.clear(std::memory_order_release);
    }

    static inline void scheduleForRun()
    {

        if (!isRunning.test_and_set(std::memory_order_acquire)) {
            ioSendNetworkThread->addTask(WrapUnique(new TaskSendBuffer));
        }
    }

    static std::atomic_flag isRunning;
    static Threading::TaskQueue<unsigned char, 16777215> dataQueue;
};

Threading::TaskQueue<unsigned char, 16777215> TaskSendBuffer::dataQueue;
std::atomic_flag TaskSendBuffer::isRunning = ATOMIC_FLAG_INIT;


size_t ordersSentCount = 0;
std::condition_variable doneCv;

class OrderCounter : public virtual Engine::LedgerDeligate, public virtual Engine::TradeDeligate {
public:
    void addedToLedger(const Engine::Order&) override
    {
        ++OrderCounter::ordersProcessed;
        if (ordersSentCount <= OrderCounter::ordersProcessed.load()) {
            doneCv.notify_all();
        }
    }

    void tradeExecuted(std::shared_ptr<Engine::Trade>) override
    {
        ++OrderCounter::tradesExecuted;
        ++OrderCounter::ordersProcessed;
        if (ordersSentCount <= OrderCounter::ordersProcessed.load()) {
            doneCv.notify_all();
        }
    }

    void orderReceived(const std::unique_ptr<Engine::Order>& order) override
    {
        ++OrderCounter::ordersReceived;
        if (ordersSentCount <= OrderCounter::ordersReceived.load()) {
            doneCv.notify_all();
        }
    }

    static std::atomic<size_t> tradesExecuted;
    static std::atomic<size_t> ordersProcessed;
    static std::atomic<size_t> ordersReceived;
};
std::atomic<size_t> OrderCounter::tradesExecuted = ATOMIC_VAR_INIT(0);
std::atomic<size_t> OrderCounter::ordersProcessed = ATOMIC_VAR_INIT(0);
std::atomic<size_t> OrderCounter::ordersReceived = ATOMIC_VAR_INIT(0);

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

        Threading::ThreadManager::setUiThread(uiThread);
        Threading::ThreadManager::setIoThread(ioThread);

        ioThread->addSocketTasker(WrapUnique(new Net::UDPSocketRecvTask));

Engine::SellLedger::instance()->setDeligate(WrapUnique(new OrderCounter));
Engine::BuyLedger::instance()->setDeligate(WrapUnique(new OrderCounter));
Engine::Trade::addDeligate(WrapUnique(new OrderCounter));

        udpSendSock = static_cast<FileDescriptor>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

        servAddr_.sin_family = AF_INET;
        servAddr_.sin_addr.s_addr = inet_addr("127.0.0.5");
        servAddr_.sin_port = htons(SERV_PORT);

        const int broadcast = 1;
        setsockopt(udpSendSock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(const int));
        // fcntl(udpSendSock, F_SETFL, O_NONBLOCK);

        const auto start = Clock::now();

        std::ifstream orderStream;
        orderStream.open("orders.log", std::ios::in | std::ios::binary | std::ios::ate);
        std::streampos size = orderStream.tellg();
        const size_t startPos = 0;
        orderStream.seekg(startPos, std::ios::beg);

        ordersSentCount = (size_t(size) - startPos) / PACKAGE_SIZE;
 //std::unique_ptr<API::DataPackage> partialPackage;

        std::vector<unsigned char> orderData(MAX_PACKET_SIZE);
        for (size_t i = size; i > startPos;) {
            size_t sz = i > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : i;
            
            orderStream.read(reinterpret_cast<char *>(orderData.data()), sz);
            // while (TaskSendBuffer::dataQueue.availableSize() < sz) {
            //     std::this_thread::sleep_for(std::chrono::microseconds(100));
            // }
            
           TaskSendBuffer::dataQueue.pushChunk(orderData, sz);
           TaskSendBuffer::scheduleForRun();

// size_t consumedLength = 0;
//             while (consumedLength < sz) {

//                 std::unique_ptr<API::DataPackage> package;
//                 if (partialPackage) {
//                     package = std::move(partialPackage);
//                 } else {
//                     package = WrapUnique(new API::DataPackage);
//                 }
//                 consumedLength += package->appendData(&orderData[consumedLength], sz - consumedLength);

//                 EXPECT_TRUE(package->isDone() || consumedLength == sz); // API Data Package has not consumed all the data and is not done

//                 if (package->isDone()) {
//                     // TODO Send to IO thread?
//                     if (UNLIKELY(!package->quickVerify())) {
//                         WARNING("QUICK VERIFY FAILED");
//                         --ordersSentCount;
//                         continue;
//                     }
//                     uiThread->addTask(WrapUnique(new Engine::ProcessOrderTask(std::move(package))));
//                     //++ordersSentCount;
//                 } else {
//                     partialPackage = std::move(package);
//                 }
//             }

            i -= sz;
        }
        orderStream.close();

//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
fprintf(stderr, "SENT: %lu of %lu\n", OrderCounter::ordersReceived.load(), ordersSentCount);
fprintf(stderr, "DataReceived: %lu, DataSent: %lu\n", Net::UDPSocketRecvTask::total_data_received_, totalDataSent);

        {
            std::mutex mux;
            std::unique_lock<std::mutex> lock(mux);
            //doneCv.wait(lock, [](){ return ordersSentCount <= OrderCounter::ordersReceived.load(); });
        }

fprintf(stderr, "DataReceived: %lu, DataSent: %lu\n", Net::UDPSocketRecvTask::total_data_received_, totalDataSent);

        const auto end = Clock::now();
        const auto timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000 / 1000;

        fprintf(stderr, "Trades Executed: %lu\n", OrderCounter::tradesExecuted.load());
        fprintf(stderr, "Orders Executed: %lu\n", OrderCounter::ordersProcessed.load());
        fprintf(stderr, "Orders Received: %lu\n", OrderCounter::ordersReceived.load());
        fprintf(stderr, "Time taken: %ld milli seconds\n", timeTaken);
        if (timeTaken / 1000 > 0)
            fprintf(stderr, "Trades per second: %lu\n", OrderCounter::tradesExecuted.load() / (timeTaken / 1000));
        fprintf(stderr, "Data Sent: %.2f mb\n", float(totalDataSent) / 1000 / 1000);
        fprintf(stderr, "Orders Per Second: %lu\n", OrderCounter::ordersProcessed.load() * 1000 / timeTaken);
        if ((float(timeTaken) / 1000) / 1000 / 1000 > 0)
            fprintf(stderr, "Bytes Per Second: %.1f mbps\n", float(totalDataSent) / (float(timeTaken) / 1000) / 1000 / 1000);
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
        // Engine::Trade::removeDeligatesForTest();
    }
    DEBUG("Exiting main function");
    return 0;
}
