#ifndef Order_h
#define Order_h

#include "../Common.h"

namespace Engine {

class Order {
public:
    typedef uint8_t[16] order_id_t;
    typedef uint64_t qty_t;
    typedef uint64_t price_t;
    typedef uint8_t flags_t;
    enum class side_t {
        BUY = 0,
        SELL = 1
    };
    enum class type_t {
        LIMIT = 0,
        MARKET = 1,
        STOP = 2
    }
    enum class Flags {
        POST_ONLY = 0x1
    }

    Order(order_id_t id, price_t price, qty_t qty, side_t side, type_t type, flags_t flags_ = 0)
        : id_(id)
        , price_(price)
        , qty_(qty)
        , side_(side)
        , type_(type)
        , flags_(flags) { }

    inline order_id_t id() const { return id_; }
    inline price_t price() const { return price_; }
    inline qty_t qty() const { return qty_; }

    inline side_t side() const { return side_; }
    inline type_t type() const { return type_; }

    inline bool isPostOnly() const { return (flags_ & Flags::POST_ONLY) == Flags::POST_ONLY; }

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
