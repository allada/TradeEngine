#ifndef Channel_h
#define Channel_h

#include <unistd.h>

template <class T>
class ThreadDataChannel : public ThreadEvent {
public:
    ThreadDataChannel()
        : socket_(static_cast<FileDescriptor>(eventfd(0, EFD_SEMAPHORE))) { }

    FileDescriptor FileDescriptor() override { return socket_; }
    void notify() override { write(socket_, &incrementer_, sizeof(incrementer_)); }
    void wait() override { read(socket_, nullptr, sizeof(uint64_t)); }

    std::unique_ptr<T> read() { return std::move(data_); }
    void send(std::unique_ptr<T> data) { data_ = std::move(data); }
private:
    static uint64_t incrementer_ = 1;
    std::unique_ptr<T> data_;
    FileDescriptor socket_;
}

#endif /* Channel_h */