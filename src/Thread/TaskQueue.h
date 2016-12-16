#ifndef TaskQueue_h
#define TaskQueue_h

#include <mutex>
#include "../Common.h"

namespace Thread {

// TODO check to see if std::move_if_noexcept is slower than making a new class to handle unique_ptr.
template <class T>
class TaskQueue {
private:
    typedef uint8_t queue_size_t;
    static const int MAX_SIZE = 1 << sizeof(queue_size_t) * 8;

public:
    int countAsReader()
    {
        queue_size_t write_index = write_index_;
        if (write_index == read_index_) {
            return 0;
        } else if (write_index > read_index_) {
            return write_index - read_index_;
        }
        return MAX_SIZE - read_index_ + write_index;
    }

    int countAsWriter()
    {
        queue_size_t read_index = read_index_;
        if (write_index_ == read_index) {
            return 0;
        } else if (write_index_ > read_index) {
            return write_index_ - read_index;
        }
        return MAX_SIZE - read_index + write_index_;
    }

    void push(T item)
    {
        // Will yield until insert point is available. This will cause high CPU usage! Maybe Sleep instead?
        std::lock_guard<std::mutex> lock(write_lock_mux_);

        while (((write_index_ + 1) % MAX_SIZE) == read_index_) {
            DEBUG("TaskQueue buffer is full, yeilding");
            std::this_thread::yield();
        }

        data_[write_index_] = std::move_if_noexcept(item);
        write_index_ = (write_index_ + 1) % MAX_SIZE;
    }

    T pop()
    {
        EXPECT_NE(static_cast<int>(read_index_), static_cast<int>(write_index_)); // There does not appear to be anything in the queue to read.
        T item = std::move_if_noexcept(data_[read_index_]);
        read_index_ = (read_index_ + 1) % MAX_SIZE;
        return std::move_if_noexcept(item);
    }

private:
    T data_[MAX_SIZE];
    std::mutex write_lock_mux_;
    queue_size_t read_index_ = 0;
    queue_size_t write_index_ = 0;

};

} /* Thread */

#endif /* TaskQueue_h */
