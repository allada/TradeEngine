#include "ThreadManager.h"
#include "Threader.h"
#include "../Common.h"
#include <thread>

using namespace Thread;

static std::unordered_map<ThreadId, std::shared_ptr<Threader>> active_threads_;
static std::unordered_set<std::shared_ptr<Threader>> stale_threads_;

static std::mutex thread_messanger_mux_;
static std::shared_ptr<Threader> main_thread_;

static std::condition_variable thread_count_change_cv_;

static thread_local std::shared_ptr<Threader> selfThread;
static thread_local std::string selfThreadName;

ThreadId ThreadManager::thisThreadId() {
    return std::this_thread::get_id();
}

std::shared_ptr<Threader> ThreadManager::thisThread()
{
    return selfThread;
}

const std::string& ThreadManager::thisThreadName()
{
    return selfThreadName;
}

ThreadId ThreadManager::mainThreadId()
{
    return main_thread_->id();
}

void ThreadManager::setSelfThread_(std::shared_ptr<Threader> thread)
{
    selfThread = thread;
}

void ThreadManager::setSelfThreadName_(const std::string& name)
{
    selfThreadName = name;
}

void ThreadManager::setMainThread(std::shared_ptr<Threader> thread)
{
    REGISTER_THREAD_COLOR();
    setSelfThreadName_(thread->name());
    setSelfThread_(thread);
    std::lock_guard<std::mutex> messanger_lock(threadManagerMux_());
    activeThreads_().emplace(thread->id(), thread);
    main_thread_ = thread;
}

std::shared_ptr<Threader> ThreadManager::getAnyNonSelfThread_()
{
    std::lock_guard<std::mutex> lock(threadManagerMux_());
    auto end = ThreadManager::activeThreads_().end();
    for (auto it = ThreadManager::activeThreads_().begin(); it != end; ++it) {
        if (it != end && it->second != thisThread()) {
            return it->second;
        }
    }
    return nullptr;
}

void ThreadManager::killAll()
{
    // TODO This is not very good, kill() may close thread completely by the time join() is called.
    std::lock_guard<std::mutex> lock(threadManagerMux_());
    auto end = ThreadManager::activeThreads_().end();
    for (auto it = ThreadManager::activeThreads_().begin(); it != end; ++it) {
        if (it->second != main_thread_) {
            it->second->kill();
        }
    }
    DEBUG("All Threads Sent Termination Signal");
}

void ThreadManager::joinAll()
{
    EXPECT_MAIN_THREAD();
    do {
        std::unique_lock<std::mutex> threadManagerLock(threadManagerMux_());
        threadCountChangeCV_().wait(threadManagerLock);
        while (staleThreads_().size()) {
            auto thread = *staleThreads_().begin();
            threadManagerLock.unlock();
            thread->join();
            threadManagerLock.lock();
        }
        threadManagerLock.unlock();
    } while (ThreadManager::activeThreads_().size() > 1);
    DEBUG("All Threads Joined");
}

void ThreadManager::untrackThread(std::shared_ptr<Threader> thread)
{
    std::lock_guard<std::mutex> lock(threadManagerMux_());
    staleThreads_().erase(thread);
}

std::unordered_map<ThreadId, std::shared_ptr<Threader>>& ThreadManager::activeThreads_()
{
    return active_threads_;
}

std::mutex& ThreadManager::threadManagerMux_()
{
    return thread_messanger_mux_;
}

std::condition_variable& ThreadManager::threadCountChangeCV_()
{
    return thread_count_change_cv_;
}

std::unordered_set<std::shared_ptr<Threader>>& ThreadManager::staleThreads_()
{
    return stale_threads_;
}
