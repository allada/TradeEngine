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

class DataPackage {
    FAST_ALLOCATE(DataPackage)
public:
    // TODO Move this into config file.
    static const uint8_t PROTOCOL_VERSION  = 1;

    enum ACTIONS {
        CREATE_BUY_ORDER = 0,
        CREATE_SELL_ORDER = 1
    };

    DataPackage() { }
    DataPackage(const Engine::Order& order)
        : data_length_(PACKAGE_SIZE)
        , is_done_(true)
    {
        data_[Version] = PROTOCOL_VERSION;
        data_[Action] = static_cast<unsigned char>(order.type() == Engine::Order::OrderType::BUY ? CREATE_BUY_ORDER : CREATE_SELL_ORDER);
        uint64_t orderId = 0; //htole64(orderId);
        uint64_t price = htole64(order.price());
        uint64_t qty = htole64(order.qty());

        memcpy(&data_[NewOrderNumber], &orderId, sizeof(uint64_t));
        memcpy(&data_[Price], &price, sizeof(uint64_t));
        memcpy(&data_[Qty], &qty, sizeof(uint64_t));
        memset(&data_[OldOrderNumber], '\0', sizeof(uint64_t));
        uint32_t checksum = htole32(::crc32(&data_[Action], PACKAGE_SIZE - Action));
        memcpy(&data_[Checksum], &checksum, sizeof(uint32_t));
    }

    const std::array<unsigned char, PACKAGE_SIZE>& data() const { return data_; }

    inline size_t appendData(const unsigned char* data, size_t len)
    {
        size_t consumeLen = std::min(PACKAGE_SIZE - data_length_, len);
        memcpy(data_.begin() + data_length_, data, consumeLen);
        data_length_ += consumeLen;
        EXPECT_GT(PACKAGE_SIZE + 1, data_length_);

        if (data_length_ >= PACKAGE_SIZE) {
            is_done_ = true;
        }

        return consumeLen;
    }

    void fail() {
        // TODO FINISH
    }

    inline bool isDone() const { return is_done_; }

    inline bool quickVerify() const
    {
        EXPECT_TRUE(is_done_);
        EXPECT_EQ(data_length_, PACKAGE_SIZE);

        if (UNLIKELY(version() != PROTOCOL_VERSION)) {
            WARNING("Got version %lu but we only support version %lu", version(), PROTOCOL_VERSION);
            return false;
        }
        return true;
    }

    inline uint8_t version() const
    {
        EXPECT_GT(data_length_, Version + sizeof(uint8_t) - 1);
        return reinterpret_cast<const uint8_t&>(data_[Version]);
    }

    inline uint32_t checksum() const
    {
        EXPECT_GT(data_length_, Checksum + sizeof(uint32_t) - 1);
        return htole32(reinterpret_cast<const uint32_t&>(data_[Checksum]));
    }

    inline uint8_t action() const
    {
        EXPECT_GT(data_length_, Action + sizeof(uint8_t) - 1);
        return reinterpret_cast<const uint8_t&>(data_[Action]);
    }

    inline uint64_t orderId() const
    {
        EXPECT_GT(data_length_, NewOrderNumber + sizeof(uint8_t) - 1);
        return reinterpret_cast<const uint8_t&>(data_[NewOrderNumber]);
    }

    inline uint64_t price() const
    {
        EXPECT_GT(data_length_, Price + sizeof(uint64_t) - 1);
        return htole64(reinterpret_cast<const uint64_t&>(data_[Price]));
    }

    inline uint64_t qty() const
    {
        EXPECT_GT(data_length_, Qty + sizeof(uint64_t) - 1);
        return htole64(reinterpret_cast<const uint64_t&>(data_[Qty]));
    }

    inline bool verifyChecksum() const
    {
        return checksum() != computeChecksum_();
    }

private:
    inline uint32_t computeChecksum_() const
    {
        return ::crc32(data_.data() + Action, PACKAGE_SIZE - Action);
    }

    enum ProtocolPositions_ {
        Version = 0,
        Checksum = 1,
        Action = 5,
        NewOrderNumber = 6,
        Price = 14,
        Qty = 22,
        OldOrderNumber = 30
    };

    std::array<unsigned char, PACKAGE_SIZE> data_;
    size_t data_length_ = 0;
    bool is_done_ = false;

};

} /* API */

#endif /* DataPackage_h */
