#ifndef TaskQueue_h
#define TaskQueue_h

#include <mutex>
#include <vector>
#include <iterator>
#include "../Common.h"

namespace Threading {

// #define COPY_OR_MOVE(data) \
//     (std::is_copy_constructible<T>::value) ? data : std::move(data);

// TODO check to see if std::move_if_noexcept is slower than making a new class to handle unique_ptr.
template <class T, std::size_t MAX_SIZE = 65536, std::size_t ACTUAL_ARRAY_SIZE = MAX_SIZE + 1>
class TaskQueue {
public:
    TaskQueue()
        : data_(ACTUAL_ARRAY_SIZE) { }

    template <typename C = T, typename std::enable_if<!std::is_copy_constructible<C>::value, int>::type = 0>
    inline T&& copyOrMove(T& data) {
        return std::move(data);
    }

    template <typename C = T, typename std::enable_if<std::is_copy_constructible<C>::value, int>::type = 0>
    inline T& copyOrMove(T& data) {
        return data;
    }

    template <class InputIterator, class OutputIterator, typename C = T, typename std::enable_if<!std::is_copy_constructible<C>::value, int>::type = 0>
    inline void copyOrMove(InputIterator first, InputIterator last, OutputIterator result) {
        std::move(first, last, result);
    }

    template <class InputIterator, class OutputIterator, typename C = T, typename std::enable_if<std::is_copy_constructible<C>::value, int>::type = 0>
    inline void copyOrMove(InputIterator first, InputIterator last, OutputIterator result) {
        std::copy(first, last, result);
    }

    size_t countAsReader()
    {
        size_t write_index = write_index_;
        if (write_index == read_index_) {
            return 0;
        } else if (write_index > read_index_) {
            return write_index - read_index_;
        }
        return ACTUAL_ARRAY_SIZE - read_index_ + write_index;
    }

    size_t countAsWriter()
    {
        size_t read_index = read_index_;
        if (write_index_ == read_index) {
            return 0;
        } else if (write_index_ > read_index) {
            return write_index_ - read_index;
        }
        return ACTUAL_ARRAY_SIZE - read_index + write_index_;
    }

    void push(T item)
    {
        // Will yield until insert point is available. This will cause high CPU usage! Maybe Sleep instead?
        std::lock_guard<std::mutex> lock(write_lock_mux_);

        while (((write_index_ + 1) % ACTUAL_ARRAY_SIZE) == read_index_) {
            DEBUG("TaskQueue buffer is full, yeilding");
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        data_[write_index_] = copyOrMove(item);
        write_index_ = (write_index_ + 1) % ACTUAL_ARRAY_SIZE;
    }

    template <std::size_t DATA_SIZE>
    void pushChunk(const std::array<T, DATA_SIZE>& data)
    {
        pushChunk(data, DATA_SIZE);
    }

    template <class ITERATOR_TYPE>
    void pushChunk(const ITERATOR_TYPE& data, size_t count) {
        EXPECT_GT(ACTUAL_ARRAY_SIZE, count);
        //static_assert(count <= MAX_SIZE, "May not pushChunk data larger than MAX_SIZE");
        std::lock_guard<std::mutex> lock(write_lock_mux_);
        while (((write_index_ + count) % ACTUAL_ARRAY_SIZE) == read_index_) {
            DEBUG("TaskQueue buffer is full, yeilding");
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        size_t endPoint = ACTUAL_ARRAY_SIZE;
        if (write_index_ + count < ACTUAL_ARRAY_SIZE) {
            endPoint = write_index_ + count;
        }
        auto first = data.begin();
        auto second = data.begin() + endPoint - write_index_;
        if (first < data.begin() || second > data.begin() + data.size()) {
            EXPECT_TRUE(false); // This should never happen!
        }
        copyOrMove(first, second, data_.begin() + write_index_);
        if (write_index_ + count > count) {
            auto first = data.begin() + endPoint - write_index_;
            auto second = data.end();
            if (first < data.begin() || second > data.begin() + data.size()) {
                EXPECT_TRUE(false); // This should never happen!
            }
            copyOrMove(first, second, data_.begin());
        }
        write_index_ = (write_index_ + count) % ACTUAL_ARRAY_SIZE;
    }

    // bool nonBlockPush(T item)
    // {
    //     // TODO These should be atomics.
    //     // Will yield until insert point is available. This will cause high CPU usage! Maybe Sleep instead?
    //     std::lock_guard<std::mutex> lock(write_lock_mux_);

    //     if (((write_index_ + 1) % ACTUAL_ARRAY_SIZE) == read_index_) {
    //         return false;
    //     }

    //     data_[write_index_] = std::move_if_noexcept(item);
    //     write_index_ = (write_index_ + 1) % ACTUAL_ARRAY_SIZE;
    //     return true;
    // }

    template <class ITERATOR_TYPE>
    void popChunk(ITERATOR_TYPE& output, size_t count)
    {
        if (UNLIKELY(!count)) {
            return;
        }
        size_t endPoint = ACTUAL_ARRAY_SIZE;
        if (read_index_ + count < ACTUAL_ARRAY_SIZE) {
            endPoint = read_index_ + count;
        }
        auto first = data_.begin() + read_index_;
        auto second = data_.begin() + endPoint;

        EXPECT_FALSE(first < data_.begin() || second > data_.begin() + data_.size());

        copyOrMove(first, second, output.begin());
        if (read_index_ + count > ACTUAL_ARRAY_SIZE) {
            first = data_.begin();
            second = data_.begin() + count - (endPoint - read_index_);

            EXPECT_FALSE(first < data_.begin() || second > data_.begin() + data_.size());

            copyOrMove(first, second, output.begin() + endPoint - read_index_);
        }
        read_index_ = (read_index_ + count) % ACTUAL_ARRAY_SIZE;
    }

    // T pop()
    // {
    //     EXPECT_NE(static_cast<int>(read_index_), static_cast<int>(write_index_)); // There does not appear to be anything in the queue to read.
    //     T item = std::move_if_noexcept(data_[read_index_]);
    //     read_index_ = (read_index_ + 1) % ACTUAL_ARRAY_SIZE;
    //     return std::move_if_noexcept(item);
    // }

private:
    std::vector<T> data_;
    std::mutex write_lock_mux_;
    size_t read_index_ = 0;
    size_t write_index_ = 0;
};

} /* Thread */

#endif /* TaskQueue_h */
