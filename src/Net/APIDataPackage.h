#ifndef APIDataPackage_h
#define APIDataPackage_h

#include <vector>
#include <algorithm>
#include "../Thread/Tasker.h"
#include "../Common.h"

namespace Net {

#define PACKAGE_SIZE 36

class APIDataPackage {
public:
    const std::vector<unsigned char>& data() const { return data_; }

    size_t appendData(const unsigned char& data, size_t len)
    {
        size_t lastDataSize = data_.size();
        size_t consumeLen = std::min(PACKAGE_SIZE - lastDataSize, len);
        data_.insert(data_.end(), &data, &data + consumeLen);

        if (data_.size() >= PACKAGE_SIZE) {
            is_done_ = true;
        }

        return consumeLen;
    }

    void fail() {
        // TODO FINISH
    }

    bool isDone() { return is_done_; }

    std::unique_ptr<Thread::Tasker> makeTask() {
        EXPECT_TRUE(is_done_);
        EXPECT_EQ(data_.size(), PACKAGE_SIZE);
        // TODO Make factory and call her to make Tasker for ui thread.
        return nullptr;
    }

private:
    std::vector<unsigned char> data_;
    bool is_done_ = 0;

};

} /* Net */

#endif /* APIDataPackage_h */
