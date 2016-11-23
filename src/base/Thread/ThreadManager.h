#ifndef ThreadManager_h
#define ThreadManager_h

#include "Threader.h"
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Thread {

inline ThreadId thisThreadId() {
    return std::this_thread::get_id();
}

std::shared_ptr<Threader> thisThread();

static std::unordered_map<ThreadId, std::shared_ptr<Threader>> active_threads_;
static std::mutex thread_messanger_mux_;
static std::unordered_map<std::thread::id, std::unique_ptr<std::thread>> unconsumed_threads;

template <typename T, typename... Args>
void entryPoint(std::mutex& mux, std::shared_ptr<T>& exportThreadObj, std::unique_ptr<std::thread>& thread, Args... args)
{
    std::shared_ptr<T> threadObj;
    {
        std::unique_lock<std::mutex> lock(mux);

        threadObj.reset(new T(std::move(thread), args...));
        DEBUG("Started %s thread", threadObj->name().c_str());

        std::lock_guard<std::mutex> messanger_lock(thread_messanger_mux_);
        active_threads_.emplace(threadObj->id(), threadObj);
        exportThreadObj = threadObj;
    }

    // threadObj->entryPoint();
    // {
    //     DEBUG("Ended %s thread", threadObj->name().c_str());
    //     std::lock_guard<std::mutex> messanger_lock(thread_messanger_mux_);
    //     // TODO Assert.
    //     active_threads_.erase(threadObj->id());
    // }
}

template <typename T, typename... Args>
static std::shared_ptr<T> createThread(Args... args)
{
    std::mutex mux;
    std::unique_ptr<std::thread> thread;
    std::shared_ptr<T> threadObj;
    std::thread::id threadId;

    {
        std::unique_lock<std::mutex> lock(mux);
        thread = WrapUnique(new std::thread(std::bind(&entryPoint<T, Args...>, std::ref(mux), std::ref(threadObj), std::ref(thread), args...)));
        threadId = thread->get_id();
    }
    std::unique_lock<std::mutex> lock(mux);
    return threadObj;
}

struct ThreadManager {
    //STATIC_ONLY(ThreadManager)
public:

    static void joinAll();
    static void killAll();
};

} /* Thread */

#endif /* ThreadManager_h */
