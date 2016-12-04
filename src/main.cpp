#include <chrono>
#include <csignal>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include "Thread/SocketPollThread.h"
#include "Thread/TaskQueueThread.h"
#include "Thread/Threader.h"
#include "Thread/ThreadManager.h"
#include "Thread/MainThread.h"

#include "Net/UDPSocketRecvTask.h"

#include "Net/TESTUDP.h"

#include <chrono>
#include <unistd.h>

std::shared_ptr<Thread::MainThread> mainThread;

std::string coin_name;

void signalHandler(int signal)
{
    EXPECT_EQ(Thread::thisThreadId(), mainThread->id());
    DEBUG("Got termination request");
    Thread::ThreadManager::killAll();
}

int main(int argc, char* argv[])
{
    mainThread = std::make_shared<Thread::MainThread>();
    Thread::ThreadManager::setMainThread(mainThread);
    {
        std::shared_ptr<Thread::TaskQueueThread> uiThread =
                Thread::createThread<Thread::TaskQueueThread>("UI");
        std::shared_ptr<Thread::SocketPollThread> ioThread =
                Thread::createThread<Thread::SocketPollThread>("IO");

        ioThread->addSocketTasker(WrapUnique(new Net::UDPSocketRecvTask));

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
