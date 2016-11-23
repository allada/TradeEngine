#include "ThreadManager.h"
#include "Threader.h"

using namespace Thread;

static std::shared_ptr<Threader> getAnyThread() {
    std::lock_guard<std::mutex> lock(thread_messanger_mux_);
    return active_threads_.size() ? active_threads_.begin()->second : nullptr;
}

std::shared_ptr<Threader> thisThread()
{
    return active_threads_.at(thisThreadId());
}

void ThreadManager::killAll()
{
    // TODO this is not thread safe and results in deadlock.
    std::lock_guard<std::mutex> lock(thread_messanger_mux_);
    for (auto it = active_threads_.begin(); it != active_threads_.end(); ++it) {
        it->second->kill();
        it->second->join();
    }
}

void ThreadManager::joinAll()
{
    while (std::shared_ptr<Threader> t = getAnyThread()) {
        t->join();
    }
}
