#include <chrono>
#include <csignal>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include "base/Thread/QueueThread.h"
#include "base/Thread/Threader.h"
#include "base/Thread/ThreadManager.h"
#include "base/Thread/MainThread.h"

Thread::MainThread mainThread;

std::string coin_name;

void signalHandler(int signal)
{
    DEBUG("Terminating");
    //net::SocketPoolTaskRunner::terminate();
    exit(signal);
}



std::shared_ptr<Thread::QueueThread> uiThread = Thread::createThread<Thread::QueueThread, std::string>("UI");
std::shared_ptr<Thread::QueueThread> ioThread = Thread::createThread<Thread::QueueThread, std::string>("IO");

int main(int argc, char* argv[])
{
    //TerminalColor::registerThread(TerminalColor::RED);
    signal(SIGINT, signalHandler);
    // Must be attached first so the threads may be closed via chanel.
    //net::SocketPoolTaskRunner::start();
    if (argc < 2 || strlen(argv[1]) < 1) {
        fprintf(stderr, "First parameter must be a valid coin abbr name.\n");
        raise(SIGINT);
    }
    coin_name = argv[1];

    Thread::ThreadManager::joinAll();
    DEBUG("All threads joined");
    return 0;
}
