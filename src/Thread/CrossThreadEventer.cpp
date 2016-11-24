#include "CrossThreadEventer.h"

#include "../Common.h"
#include <sys/eventfd.h>
#include <unistd.h>

using namespace Thread;

static const uint64_t incrementer_ = 1;

CrossThreadEventer::CrossThreadEventer()
    : socket_(static_cast<FileDescriptor>(eventfd(0, EFD_SEMAPHORE)))
{
}

void CrossThreadEventer::notify() {
    ::write(socket_, &incrementer_, sizeof(incrementer_));
}

void CrossThreadEventer::wait() {
    ::read(socket_, nullptr, sizeof(uint64_t));
}

void CrossThreadEventer::close() {
    ::close(socket_);
}

void CrossThreadEventer::error(int err) {
    WARNING("Error on socket: %d, with errorCode: %d", static_cast<int>(socket_), err);
}
