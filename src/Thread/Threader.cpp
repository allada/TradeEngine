#include "Threader.h"

#include "ThreadManager.h"

using namespace Thread;

ThreadId Thread::thisThreadId()
{
    return ThreadManager::thisThreadId();
}

std::shared_ptr<Threader> Thread::thisThread()
{
    return ThreadManager::thisThread();
}

const std::string& Thread::thisThreadName()
{
    return ThreadManager::thisThreadName();
}
