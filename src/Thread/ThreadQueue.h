#ifndef ThreadQueue_h
#define ThreadQueue_h

#include <mutex>
#include "../Common.h"

namespace Thread {

template <class T>
class ThreadQueue {
private:
    typedef uint8_t queue_size_t;
    static const int MAX_SIZE = 1 << sizeof(queue_size_t) * 8;
public:
    int count()
    {
        std::lock_guard<std::mutex> lock(write_lock_mux_);
        if (write_index_ == read_index_) {
            return 0;
        } else if (write_index_ > read_index_) {
            return write_index_ - read_index_;
        }
        return MAX_SIZE - read_index_ + write_index_;
    }

    void push(T item)
    {
        // Will yield until insert point is available. This will cause high CPU usage! Maybe Sleep instead?
        std::lock_guard<std::mutex> lock(write_lock_mux_);

        while (((write_index_ + 1) % MAX_SIZE) == read_index_) {
            DEBUG("ThreadQueue buffer is full, yeilding");
            std::this_thread::yield();
        }

        data_[++write_index_] = item;
    }

    T pop()
    {
        ASSERT_NE(read_index_, write_index_, "There does not appear to be anything in the queue to read.");
        T item = data_[read_index_];
        read_index_ = (read_index_ + 1) % MAX_SIZE;
        return item;
    }

private:
    T data_[MAX_SIZE];
    std::mutex write_lock_mux_;
    queue_size_t read_index_ = 0;
    queue_size_t write_index_ = 0;
};

} /* Thread */

#endif /* ThreadQueue_h */
