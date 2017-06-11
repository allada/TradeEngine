#include "Threader.h"

#include "ThreadManager.h"

using namespace Threading;

ThreadId Threading::thisThreadId()
{
    return ThreadManager::thisThreadId();
}

std::shared_ptr<Threader> Threading::thisThread()
{
    return ThreadManager::thisThread();
}

const std::string& Threading::thisThreadName()
{
    return ThreadManager::thisThreadName();
}
