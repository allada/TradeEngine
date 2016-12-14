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

#define PACKAGE_SIZE 36

// static std::array<APIOrderTask, 6> = {
//     APICreateOrder
// };

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

        uint32_t checksum = computeChecksum();
        if (checksum != extractChecksum()) {
            WARNING("Got bad checksum, ignoring packet");
            return nullptr;
        }

        if (extractVersion() != PROTOCOL_VERSION) {
            WARNING("Got version %d but we only support version %d", extractVersion(), PROTOCOL_VERSION);
            return nullptr;
        }




        return nullptr;
    }

private:
    inline uint8_t extractVersion()
    {
        EXPECT_GT(data_.size(), 0);
        return static_cast<uint8_t>(data_[0]);
    }

    inline uint16_t extractChecksum()
    {
        EXPECT_GT(data_.size(), 2);
        return data_[1] << 8 | data_[2];
    }

    inline uint8_t extractType()
    {
        EXPECT_GT(data_.size(), 3);
        return data_[3];
    }

    inline uint32_t computeChecksum()
    {
        return ::crc32(data_.data() + 3, data_.size());
    }

    std::vector<unsigned char> data_;
    bool is_done_ = 0;

};

} /* Net */

#endif /* APIDataPackage_h */
