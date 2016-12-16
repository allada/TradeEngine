#ifndef APIDataPackage_h
#define APIDataPackage_h

#include <vector>
#include <algorithm>
#include "../Engine/Order.h"
#include "../Thread/Tasker.h"
#include "../Common.h"
#include "../includes/crc32.h"
#include <endian.h>
#include <cstring>


// TODO Move this into config file.
#define PROTOCOL_VERSION 1

namespace Net {

#define PACKAGE_SIZE 38

#define ACTION_CREATE_BUY_ORDER 1
#define ACTION_CREATE_SELL_ORDER 2


class APIDataPackage {
public:
    APIDataPackage() { }
    APIDataPackage(const Engine::Order& order)
        : data_length_(PACKAGE_SIZE)
        , is_done_(true)
    {
        data_[Version] = PROTOCOL_VERSION;
        data_[Action] = order.type() == Engine::Order::OrderType::BUY ? ACTION_CREATE_BUY_ORDER : ACTION_CREATE_SELL_ORDER;
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

    const std::array<unsigned char, 38>& data() const { return data_; }

    size_t appendData(const unsigned char& data, size_t len)
    {
        size_t consumeLen = std::min(PACKAGE_SIZE - data_length_, len);
        memcpy(data_.data(), &data, consumeLen);
        data_length_ += consumeLen;

        if (data_length_ >= PACKAGE_SIZE) {
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
    enum ProtocolPositions_ {
        Version = 0,
        Checksum = 1,
        Action = 5,
        NewOrderNumber = 6,
        Price = 14,
        Qty = 22,
        OldOrderNumber = 30
    };

    uint8_t version_() const
    {
        EXPECT_GT(data_length_, Version + sizeof(uint8_t) - 1);
        return reinterpret_cast<const uint8_t&>(data_[Version]);
    }

    uint32_t checksum_() const
    {
        EXPECT_GT(data_length_, Checksum + sizeof(uint32_t) - 1);
        return htole32(reinterpret_cast<const uint32_t&>(data_[Checksum]));
    }

    uint8_t action_() const
    {
        EXPECT_GT(data_length_, Action + sizeof(uint8_t) - 1);
        return reinterpret_cast<const uint8_t&>(data_[Action]);
    }

    uint64_t orderId_() const
    {
        EXPECT_GT(data_length_, NewOrderNumber + sizeof(uint8_t) - 1);
        return reinterpret_cast<const uint8_t&>(data_[NewOrderNumber]);
    }

    uint64_t price_() const
    {
        EXPECT_GT(data_length_, Price + sizeof(uint64_t) - 1);
        return htole64(reinterpret_cast<const uint64_t&>(data_[Price]));
    }

    uint64_t qty_() const
    {
        EXPECT_GT(data_length_, Qty + sizeof(uint64_t) - 1);
        return htole64(reinterpret_cast<const uint64_t&>(data_[Qty]));
    }

    uint32_t computeChecksum_() const
    {
        return ::crc32(data_.data() + Action, data_.size() - Action);
    }

    std::array<unsigned char, PACKAGE_SIZE> data_;
    size_t data_length_ = 0;
    bool is_done_ = false;

};

} /* Net */

#endif /* APIDataPackage_h */
