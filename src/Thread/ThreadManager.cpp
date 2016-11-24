#include "ThreadManager.h"
#include "Threader.h"
#include "../Common.h"
#include <thread>

using namespace Thread;

static std::unordered_map<ThreadId, std::shared_ptr<Threader>> active_threads_;
static std::mutex thread_messanger_mux_;

static thread_local std::shared_ptr<Threader> selfThread;

ThreadId ThreadManager::thisThreadId() {
    return std::this_thread::get_id();
}

std::shared_ptr<Threader> ThreadManager::thisThread()
{
    return selfThread;
}

void ThreadManager::setSelfThread_(std::shared_ptr<Threader> thread)
{
    selfThread = thread;
}

std::shared_ptr<Threader> ThreadManager::getAnyNonSelfThread_()
{
    std::lock_guard<std::mutex> lock(threadManagerMux_());
    for (auto it = ThreadManager::activeThreads_().begin(); it != ThreadManager::activeThreads_().end(); ++it) {
        if (it->second != thisThread()) {
            return it->second;
        }
    }
    return nullptr;
}

void ThreadManager::killAll()
{
    // TODO This is not very good, kill() may close thread completely by the time join() is called.
    std::shared_ptr<Threader> thread = getAnyNonSelfThread_();
    while (thread) {
        thread->kill();
        thread->join();
        thread = getAnyNonSelfThread_();
    }
    DEBUG("All Threads Terminated");
}

void ThreadManager::joinAll()
{
    std::shared_ptr<Threader> thread = getAnyNonSelfThread_();
    while (thread) {
        thread->join();
        thread = getAnyNonSelfThread_();
    }
}


std::unordered_map<ThreadId, std::shared_ptr<Threader>>& ThreadManager::activeThreads_()
{
    return active_threads_;
}

std::mutex& ThreadManager::threadManagerMux_()
{
    return thread_messanger_mux_;
}