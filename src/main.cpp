#include <chrono>
#include <csignal>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include "Thread/QueueThread.h"
#include "Thread/Threader.h"
#include "Thread/ThreadManager.h"
#include "Thread/MainThread.h"

std::shared_ptr<Thread::MainThread> mainThread;

std::string coin_name;

void signalHandler(int signal)
{
    ASSERT_EQ(Thread::thisThreadId(), mainThread->id(), "signalHandler must be triggerd from mainThread only");
    DEBUG("Terminating");
    Thread::ThreadManager::killAll();
    DEBUG("Main thread closing");
    exit(signal);
}

int main(int argc, char* argv[])
{
    mainThread = std::make_shared<Thread::MainThread>();
    Thread::ThreadManager::setMainThread(mainThread);

    std::shared_ptr<Thread::QueueThread> uiThread = Thread::createThread<Thread::QueueThread>("UI");
    std::shared_ptr<Thread::QueueThread> ioThread = Thread::createThread<Thread::QueueThread>("IO");


    //TerminalColor::registerThread(TerminalColor::RED);
    signal(SIGINT, signalHandler);
    // Must be attached first so the threads may be closed via chanel.
    //net::SocketPoolTaskRunner::start();
    if (argc < 2 || strlen(argv[1]) < 1) {
        fprintf(stderr, "First parameter must be a valid coin abbr name.\n");
        raise(SIGINT);
    }
    coin_name = argv[1];
    Thread::ThreadManager::killAll();
    exit(1);
    Thread::ThreadManager::joinAll();
    DEBUG("All threads joined");
    return 0;
}
