#include "Thread.h"

#include "ThreadManager.h"

using namespace Threading;

void Thread::join()
{
    if (thread_->joinable())
        thread_->join();
    ThreadManager::untrackThread(shared_from_this());
}
