#ifndef ThreadManager_h
#define ThreadManager_h

#include "Threader.h"
#include "SocketPollThread.h"
#include "TaskQueueThread.h"
#include "../Common.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace Threading {

inline static std::shared_ptr<SocketPollThread> ioThread();
inline static std::shared_ptr<TaskQueueThread> uiThread();

inline static std::unordered_map<ThreadId, std::shared_ptr<Threader>>& activeThreads_();
inline static std::mutex& threadManagerMux_();
inline static void setSelfThread_(std::shared_ptr<Threader>);
inline static void setSelfThreadName_(const std::string& name);
inline static std::condition_variable& threadCountChangeCV_();
inline static std::unordered_set<std::shared_ptr<Threader>>& staleThreads_();

struct ThreadManager {
    STATIC_ONLY(ThreadManager)
public:
    static ThreadId mainThreadId();
    static ThreadId thisThreadId();
    static std::shared_ptr<Threader> thisThread();
    static const std::string& thisThreadName();

    static void setMainThread(std::shared_ptr<Threader> thread);
    static void setIoThread(std::shared_ptr<SocketPollThread> thread);
    static void setUiThread(std::shared_ptr<TaskQueueThread> thread);

    static void joinAll();
    static void killAll();

    static void untrackThread(std::shared_ptr<Threader>);

    inline static int activeThreadsCount()
    {
        std::lock_guard<std::mutex> messanger_lock(threadManagerMux_());
        return activeThreads_().size();
    }

private:
    friend std::unordered_map<ThreadId, std::shared_ptr<Threader>>& activeThreads_();
    friend std::mutex& threadManagerMux_();
    friend void setSelfThread_(std::shared_ptr<Threader>);
    friend std::condition_variable& threadCountChangeCV_();
    friend std::unordered_set<std::shared_ptr<Threader>>& staleThreads_();
    friend void setSelfThreadName_(const std::string& name);    
    friend std::shared_ptr<SocketPollThread> ioThread();
    friend std::shared_ptr<TaskQueueThread> uiThread();

    static std::shared_ptr<SocketPollThread> ioThread_();
    static std::shared_ptr<TaskQueueThread> uiThread_();

    static std::unordered_map<ThreadId, std::shared_ptr<Threader>>& activeThreads_();
    static std::mutex& threadManagerMux_();

    static std::condition_variable& threadCountChangeCV_();

    static std::unordered_set<std::shared_ptr<Threader>>& staleThreads_();

    static void setSelfThread_(std::shared_ptr<Threader>);
    static void setSelfThreadName_(const std::string& name);
};

inline static std::shared_ptr<SocketPollThread> ioThread()
{
    return ThreadManager::ioThread_();
}

inline static std::shared_ptr<TaskQueueThread> uiThread()
{
    return ThreadManager::uiThread_();
}

inline static std::mutex& threadManagerMux_()
{
    return ThreadManager::threadManagerMux_();
}

inline static std::condition_variable& threadCountChangeCV_()
{
    return ThreadManager::threadCountChangeCV_();
}

inline static void setSelfThread_(std::shared_ptr<Threader> thread)
{
    return ThreadManager::setSelfThread_(thread);
}

inline static void setSelfThreadName_(const std::string& name)
{
    ThreadManager::setSelfThreadName_(name);
}

inline static std::unordered_set<std::shared_ptr<Threader>>& staleThreads_()
{
    return ThreadManager::staleThreads_();
}

inline static std::unordered_map<ThreadId, std::shared_ptr<Threader>>& activeThreads_()
{
    return ThreadManager::activeThreads_();
}

template <typename T>
void entryPoint(std::mutex& mux, std::condition_variable& cv, std::shared_ptr<T>& exportThreadObj, std::unique_ptr<std::thread>& thread, std::string name)
{
    std::shared_ptr<T> threadObj;
    {
        REGISTER_THREAD_COLOR();
        setSelfThreadName_(name);
        std::lock_guard<std::mutex> lock(mux);

        threadObj.reset(new T(std::move(thread), name));
        setSelfThread_(threadObj);

        std::lock_guard<std::mutex> messanger_lock(threadManagerMux_());
        activeThreads_().emplace(threadObj->id(), threadObj);
        DEBUG("Started %s thread", threadObj->name().c_str());
        exportThreadObj = threadObj;
        cv.notify_one();
    }
    threadCountChangeCV_().notify_all();
    threadObj->entryPoint();
    {
        std::lock_guard<std::mutex> messanger_lock(threadManagerMux_());
        activeThreads_().erase(threadObj->id());
        staleThreads_().emplace(threadObj);
    }
    threadCountChangeCV_().notify_all();
    DEBUG("Fully terminated %s", threadObj->name().c_str());
}

template <typename T>
static std::shared_ptr<T> createThread(const std::string& name)
{
    EXPECT_MAIN_THREAD();
    DEBUG("Starting %s thread", name.c_str());
    std::mutex mux;
    std::condition_variable cv;
    std::unique_ptr<std::thread> thread;
    std::shared_ptr<T> threadObj;
    std::thread::id threadId;
    {
        std::unique_lock<std::mutex> lock(mux);
        thread = WrapUnique(new std::thread(std::bind(&entryPoint<T>, std::ref(mux), std::ref(cv), std::ref(threadObj), std::ref(thread), name)));
        threadId = thread->get_id();
    }
    std::unique_lock<std::mutex> lock(mux);
    // Wait for the thread to construct the threadObj (it may be null here, this waits for it to be set)
    cv.wait(lock, [&threadObj]() { return threadObj != nullptr; });
    DEBUG("Got Thread Object");
    return threadObj;
}

} /* Thread */

#endif /* ThreadManager_h */
