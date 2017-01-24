#ifndef Order_h
#define Order_h

#include "../Common.h"
#include <cstring>

namespace Engine {

class Order {
public:
    enum class Flags {
        POST_ONLY = 0x1
    };
    struct order_id_t {
    public:
        order_id_t(const unsigned char& data)
        {
            memcpy(&data_, &data, 16);
        }

        unsigned char& operator[](size_t idx) { return data_[idx]; }
        const unsigned char& operator[](size_t idx) const { return data_[idx]; }

    private:
        std::array<unsigned char, 16> data_;

    };
    // typedef uint8_t order_id_t[16];
    typedef uint64_t qty_t;
    typedef uint64_t price_t;
    struct flags_t {
    public:
        flags_t(uint8_t flag)
            : flag_(flag) { }
        flags_t(const Flags& flag)
            : flag_(static_cast<uint8_t>(flag)) { }
        flags_t operator |(const Flags& flag) const { return flags_t(flag_ | static_cast<uint8_t>(flag)); }
        flags_t operator &(const Flags& flag) const { return flags_t(flag_ & static_cast<uint8_t>(flag)); }
        bool operator ==(const Flags& flag) const { return flag_ == static_cast<uint8_t>(flag); }
        flags_t& operator |=(const Flags& flag) { flag_ |= static_cast<uint8_t>(flag); return *this; }
        flags_t& operator &=(const Flags& flag) { flag_ &= static_cast<uint8_t>(flag); return *this; }

    private:
        uint8_t flag_;

    };
    enum class side_t {
        BUY = 0,
        SELL = 1
    };
    enum class type_t {
        LIMIT = 0,
        MARKET = 1,
        // STOP = 2
    };

    Order(const order_id_t& id, price_t price, qty_t qty, side_t side, type_t type, flags_t flags = 0)
        : id_(id)
        , price_(price)
        , qty_(qty)
        , side_(side)
        , type_(type)
        , flags_(flags) { }

    const order_id_t& id() const { return id_; }
    price_t price() const { return price_; }
    qty_t qty() const { return qty_; }

    side_t side() const { return side_; }
    type_t type() const { return type_; }

    bool isPostOnly() const { return (flags_ & Flags::POST_ONLY) == Flags::POST_ONLY; }

private:
    order_id_t id_;
    price_t price_;
    qty_t qty_;
    side_t side_;
    type_t type_;
    flags_t flags_;

};

} /* Engine */

#endif /* Order_h */
