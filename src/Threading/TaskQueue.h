#ifndef TaskQueue_h
#define TaskQueue_h

#include <mutex>
#include <vector>
#include <iterator>
#include <atomic>
#include "../Common.h"

namespace Threading {

template <class T, std::size_t MAX_SIZE = 65536, std::size_t ACTUAL_ARRAY_SIZE = MAX_SIZE + 1>
class TaskQueue {
public:
    TaskQueue()
        : data_(new std::array<T, MAX_SIZE>)
        , lock_(write_lock_mux_, std::defer_lock) { }

    template <typename C = T, typename std::enable_if<!std::is_copy_constructible<C>::value, int>::type = 0>
    inline T&& copyOrMove(T& data)
    {
        return std::move(data);
    }

    template <typename C = T, typename std::enable_if<std::is_copy_constructible<C>::value, int>::type = 0>
    inline T& copyOrMove(T& data) const
    {
        return data;
    }

    template <class InputIterator, class OutputIterator, typename C = T, typename std::enable_if<!std::is_copy_constructible<C>::value, int>::type = 0>
    inline void copyOrMove(InputIterator first, InputIterator last, OutputIterator result)
    {
        std::move(first, last, result);
    }

    template <class InputIterator, class OutputIterator, typename C = T, typename std::enable_if<std::is_copy_constructible<C>::value, int>::type = 0>
    inline void copyOrMove(InputIterator first, InputIterator last, OutputIterator result)
    {
        std::copy(first, last, result);
    }

    inline size_t countAsReader() const
    {
        return count_;
    }

    inline size_t countAsWriter() const
    {
        return count_;
    }

    inline void yieldUntilDataFits(const size_t sz) const
    {
        while (MAX_SIZE - countAsWriter() < sz) {
            //DEBUG("TaskQueue buffer is full, yeilding, size: %lu", MAX_SIZE);
            std::this_thread::yield();
        }
    }

    inline void push(T item)
    {
        // Will yield until insert point is available. This will cause high CPU usage! Maybe Sleep instead?
        lock_.lock();

        yieldUntilDataFits(1);

        (*data_)[write_index_] = copyOrMove(item);
        write_index_ = (write_index_ + 1) % ACTUAL_ARRAY_SIZE;
        ++count_;
        lock_.unlock();
    }

    template <std::size_t DATA_SIZE>
    inline void pushChunk(const std::array<T, DATA_SIZE>& data)
    {
        pushChunk(data, DATA_SIZE);
    }

    template <class ITERATOR_TYPE>
    inline void pushChunk(const ITERATOR_TYPE& data, size_t count) {
        EXPECT_GT(ACTUAL_ARRAY_SIZE, count);
        lock_.lock();

        yieldUntilDataFits(count);

        size_t endPoint = ACTUAL_ARRAY_SIZE;
        if (write_index_ + count < ACTUAL_ARRAY_SIZE) {
            endPoint = write_index_ + count;
        }
        auto inputBegin = data_->begin();
        auto first = data.begin();
        auto second = data.begin() + endPoint - write_index_;

        EXPECT_FALSE(first < data.begin() || second > data.begin() + data.size());

        copyOrMove(first, second, inputBegin + write_index_);
        auto processedSize = second - first;
        if (write_index_ + count > ACTUAL_ARRAY_SIZE) {
            auto first = data.begin() + processedSize;
            auto second = data.end();

            EXPECT_FALSE(first < data.begin() || second > data.begin() + data.size());

            copyOrMove(first, second, inputBegin);
        }
        write_index_ = (write_index_ + count) % ACTUAL_ARRAY_SIZE;
        count_ += count;
        lock_.unlock();
    }

    template <class ITERATOR_TYPE>
    inline void popChunk(ITERATOR_TYPE& output, size_t count)
    {
        if (!count) {
            return;
        }
        size_t endPoint = ACTUAL_ARRAY_SIZE;
        if (read_index_ + count < ACTUAL_ARRAY_SIZE) {
            endPoint = read_index_ + count;
        }
        auto begin = data_->begin();
        auto first = begin + read_index_;
        auto second = begin + endPoint;

        EXPECT_FALSE(first < begin || second > begin + ACTUAL_ARRAY_SIZE);

        copyOrMove(first, second, output.begin());
        auto processedSize = second - first;
        if (read_index_ + count > ACTUAL_ARRAY_SIZE) {
            first = begin;
            second = begin + count - processedSize;

            EXPECT_FALSE(first < begin || second > begin + ACTUAL_ARRAY_SIZE);

            copyOrMove(first, second, output.begin() + processedSize);
        }
        read_index_ = (read_index_ + count) % ACTUAL_ARRAY_SIZE;
        count_ -= count;
    }

private:
    std::unique_ptr<std::array<T, MAX_SIZE>> data_;
    std::mutex write_lock_mux_;
    std::unique_lock<std::mutex> lock_;
    std::atomic<size_t> count_ = ATOMIC_VAR_INIT(0);
    size_t read_index_ = 0;
    size_t write_index_ = 0;

};

} /* Thread */

#endif /* TaskQueue_h */
