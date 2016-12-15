#ifndef APIDataPackage_h
#define APIDataPackage_h

#include <vector>
#include <algorithm>
#include "../Thread/Tasker.h"
#include "../Common.h"
#include "../includes/crc32.h"


// TODO Move this into config file.
#define PROTOCOL_VERSION 1

namespace Net {

#define PACKAGE_SIZE 38

#define ACTION_CREATE_BUY_ORDER 1
#define ACTION_CREATE_SELL_ORDER 2

class APIDataPackage {
public:
    const std::vector<unsigned char>& data() const { return data_; }

    size_t appendData(const unsigned char& data, size_t len)
    {
        size_t lastDataSize = data_.size();
        size_t consumeLen = std::min(PACKAGE_SIZE - lastDataSize, len);
        data_.insert(data_.end(), &data, &data + consumeLen);

        if (data_.size() >= PACKAGE_SIZE) {
            DBG();
            is_done_ = true;
        }

        return consumeLen;
    }

    void fail() {
        // TODO FINISH
    }

    bool isDone() const { return is_done_; }

    std::unique_ptr<Thread::Tasker> makeTask();

private:
    inline uint8_t extractVersion()
    {
        DBG("version: %x", reinterpret_cast<uint8_t&>(data_[0]));
        EXPECT_GT(data_.size(), 0);
        return reinterpret_cast<uint8_t>(data_[0]);
    }

    inline uint32_t extractChecksum()
    {
        DBG("in checksum: %x", reinterpret_cast<uint32_t&>(data_[1]));
        EXPECT_GT(data_.size(), 3);
        return reinterpret_cast<uint32_t&>(data_[1]); //data_[1] << 8 | data_[2];
    }

    inline uint8_t extractAction()
    {
        DBG("in action: %x", data_[5]);
        EXPECT_GT(data_.size(), 4);
        return reinterpret_cast<uint8_t&>(data_[5]);
    }

    inline uint64_t extractPrice()
    {
        EXPECT_GT(data_.size(), 21);
        return reinterpret_cast<uint64_t&>(data_[14]);
    }

    inline uint64_t extractQty()
    {
        EXPECT_GT(data_.size(), 29);
        return reinterpret_cast<uint64_t&>(data_[22]);
    }

    inline uint32_t computeChecksum()
    {
        return ::crc32(data_.data() + 5, data_.size() - 5);
    }

    std::vector<unsigned char> data_;
    bool is_done_ = false;

};

} /* Net */

#endif /* APIDataPackage_h */
