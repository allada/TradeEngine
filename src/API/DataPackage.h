#ifndef DataPackage_h
#define DataPackage_h

#include <vector>
#include <algorithm>
#include "../Engine/Order.h"
#include "../Threading/Tasker.h"
#include "../Common.h"
#include "../includes/crc32.h"
#include <endian.h>
#include <cstring>

namespace API {

#define PACKAGE_SIZE 38
// TODO Move this into config file.
#define PROTOCOL_VERSION 1    

class DataPackageType;

class DataPackage {
    FAST_ALLOCATE(DataPackage)
public:
    enum class NibblesPerPosition_ {
        PROTOCOL_VERSION = 2,
        CHECKSUM         = 8,
        ACTION_TYPE      = 2
    };

    enum class ProtocolPositions_ {
        PROTOCOL_VERSION = 0, // +1
        CHECKSUM         = 1, // +4
        ACTION_TYPE      = 5, // +1

        PACKAGE_END      = 6  // +0
    };

    enum class action_type_t {
        CREATE_ORDER = 0,
        REMOVE_ORDER = 1
    };

    DataPackage() { }

    // const std::array<unsigned char, PACKAGE_SIZE>& data() const { return data_; }

    inline size_t appendData(const unsigned char* data, size_t len)
    {
        if (action_ != nullptr) {
            return action_->appendData(data, len);
        }
        size_t consumeLen = std::min(ProtocolPositions_::PACKAGE_END - data_length_, len);
        memcpy(data_.begin() + data_length_, data, consumeLen);
        data_length_ += consumeLen;

        if (UNLIKELY(data_length_ < ProtocolPositions_::PACKAGE_END)) {
            return consumeLen;
        }

        if (LIKELY(action_ == nullptr)) {
            uint8_t packageType = reinterpret_cast<const uint8_t&>(data_[ProtocolPositions_::ACTION_TYPE]);
            switch (packageType) {
                case action_type_t::CREATE_ORDER:
                    action_ = WrapUnique(CreateOrderPackage());
                    break;
                case action_type_t::REMOVE_ORDER:
                    EXPECT_TRUE(false);
                    WARNING("Remove order not implemented yet");
                    return -1;
                default:
                    EXPECT_TRUE(false);
                    WARNING("Got package with unknown type: %u", packageType);
                    return -1;
            }
        }

        consumeLen += appendData(data + consumeLen, len - consumeLen);
        return consumeLen;
    }

    inline bool isDone() const { return action_ ? action_->isDone() : false; }

    inline bool quickVerify() const
    {
        EXPECT_TRUE(isDone());
        EXPECT_NE(action_, nullptr);

        if (UNLIKELY(version_() != PROTOCOL_VERSION)) {
            WARNING("Got version %lu but we only support version %lu", version_(), PROTOCOL_VERSION);
            return false;
        }

        return action_->quickVerify();
    }

    inline uint8_t version_() const { return reinterpret_cast<const uint8_t&>(data_[ProtocolPositions_::PROTOCOL_VERSION]); }

    // inline uint32_t checksum() const
    // {
    //     EXPECT_GT(data_length_, Checksum + sizeof(uint32_t) - 1);
    //     return htole32(reinterpret_cast<const uint32_t&>(data_[Checksum]));
    // }

    // inline uint8_t action() const
    // {
    //     EXPECT_GT(data_length_, Action + sizeof(uint8_t) - 1);
    //     return reinterpret_cast<const uint8_t&>(data_[Action]);
    // }

    // inline uint64_t orderId() const
    // {
    //     EXPECT_GT(data_length_, NewOrderNumber + sizeof(uint64_t) - 1);
    //     return reinterpret_cast<const uint64_t&>(data_[NewOrderNumber]);
    // }

    // inline uint64_t price() const
    // {
    //     EXPECT_GT(data_length_, Price + sizeof(uint64_t) - 1);
    //     return htole64(reinterpret_cast<const uint64_t&>(data_[Price]));
    // }

    // inline uint64_t qty() const
    // {
    //     EXPECT_GT(data_length_, Qty + sizeof(uint64_t) - 1);
    //     return htole64(reinterpret_cast<const uint64_t&>(data_[Qty]));
    // }

    // inline bool verifyChecksum() const
    // {
    //     return checksum() != computeChecksum_();
    // }

private:
    // inline uint32_t computeChecksum_() const
    // {
    //     return ::crc32(data_.data() + Action, ProtocolPositions_::PACKAGE_END - Action);
    // }

    std::array<unsigned char, ProtocolPositions_::PACKAGE_END> data_;
    size_t data_length_ = 0;
    std::unique_ptr<DataPackageType> action_;

};

class DataPackageType {
    virtual bool isDone() = 0;
    virtual bool quickVerify() = 0;
    virtual size_t appendData(const unsigned char* data, size_t len) = 0;
    virtual size_t makeTask() = 0;

};

} /* API */

#endif /* DataPackage_h */
