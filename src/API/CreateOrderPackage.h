#ifndef CreateOrderPackage_h
#define CreateOrderPackage_h

#include "../Engine/Order.h"
#include "DataPackage.h"
#include "../Common.h"

namespace API {

class CreateOrderPackage : public virtual DataPackageType {
    FAST_ALLOCATE(CreateOrderPackage)

private:
    enum class NibblesPerPosition_ {
        SIDE         = 1,
        ORDER_TYPE   = 1,
        FLAGS        = 2,
        ORDER_ID     = 32,
        PRICE        = 16,
        QTY          = 16,
    };

    enum class ProtocolPositions_ {
        SIDE         = 0,  // Note this is bitshifted >> 4.
        ORDER_TYPE   = 0,  // Note this is masked 0xF.
        FLAGS        = 1,  // +1
        ORDER_ID     = 2,  // +16
        PRICE        = 18, // +8
        QTY          = 26, // +8

        PACKAGE_END  = 34  // +0
    };

    enum class order_side_t {
        BUY  = 0,
        SELL = 1
    };

    enum class order_type_t {
        LIMIT  = 0,
        MARKET = 1,
        STOP   = 2
    }

    enum class Flags {
        POST_ONLY = 0x1
    }

    static order_side_t sideFromOrder_(const Engine::Order& order)
    {
        return order.side() == Engine::Order::side_t::BUY ? order_side_t::BUY : order_side_t::SELL;
    }

    static order_type_t typeFromOrder_(const Engine::Order& order)
    {
        switch (order.type()) {
            case Order::type_t::LIMIT:
                return order_type_t::LIMIT;
            case Order::type_t::MARKET:
                return order_type_t::MARKET;
            case Order::type_t::STOP:
                return order_type_t::STOP;
        }
    }

public:

    CreateOrderPackage() { }
    CreateOrderPackage(const Engine::Order& order)
        : data_length_(ProtocolPositions_::PACKAGE_END)
    {
        order_side_t      side    = sieFromOrder_(order);
        order_type_t      type    = typeFromOrder_(order);
        uint8_t           flags   = order.isPostOnly() ? Flags::POST_ONLY : 0;
        Order::order_id_t orderId = order.id();
        uint64_t          price   = htole64(order.price());
        uint64_t          qty     = htole64(order.qty());

        uint8_t sizeAndType = (side << 4) | (type & 0xF);

        memcpy(&data_[ProtocolPositions_::SIDE],     &sideAndType, (NibblesPerPosition_::SIDE + NibblesPerPosition_::ORDER_TYPE) / 2);
        // No need to put OrderType here because it's done above.
        memcpy(&data_[ProtocolPositions_::FLAGS],    &flags,       NibblesPerPosition_::FLAGS / 2);
        memcpy(&data_[ProtocolPositions_::ORDER_ID], &orderId,     NibblesPerPosition_::ORDER_ID / 2);
        memcpy(&data_[ProtocolPositions_::PRICE],    &price,       NibblesPerPosition_::PRICE / 2);
        memcpy(&data_[ProtocolPositions_::QTY],      &qty,         NibblesPerPosition_::QTY / 2);
    }

    const std::array<unsigned char, ProtocolPositions_::PACKAGE_END>& data() const { return data_; }

    inline size_t appendData(const unsigned char* data, size_t len)
    {
        size_t consumeLen = std::min(PACKAGE_SIZE - data_length_, len);
        memcpy(data_.begin() + data_length_, data, consumeLen);
        data_length_ += consumeLen;
        EXPECT_GT(PACKAGE_SIZE + 1, data_length_);

        return consumeLen;
    }

    inline bool isDone() const { return data_length_ == ProtocolPositions_::PACKAGE_END; }

    bool quickVerify() const
    {
        EXPECT_TRUE(isDone());
        if (UNLIKELY(price_() <= 0)) {
            WARNING("Price is %lu", price());
            return false;
        }
        if (UNLIKELY(qty_() <= 0)) {
            WARNING("Qty is %lu", qty());
            return false;
        }
        return true;
    }

    inline std::unique_ptr<Engine::Order> makeOrder() const
    {
        EXPECT_TRUE(isDone());

        Engine::Order::order_id_t orderId = reinterpret_cast<const Engine::Order::order_id_t&>(data_[ProtocolPositions_::ORDER_ID]);
        Engine::Order::price_t    price   = price_();
        Engine::Order::qty_t      qty     = qty_();
        Engine::Order::side_t     side    = reinterpret_cast<const Engine::Order::side_t&>(    (data_[ProtocolPositions_::SIDE] >> 4));
        Engine::Order::type_t     type    = reinterpret_cast<const Engine::Order::type_t&>(    (data_[ProtocolPositions_::TYPE] & 0xF));
        Engine::Order::flags_t    flags   =  reinterpret_cast<const Engine::Order::flags_t&>(  data_[ProtocolPositions_::FLAGS]);

        return WrapUnique(new Engine::Order(orderId, price, qty, side, type, flags));
    }

private:
    inline Engine::Order::price_t price_() const { return static_cast<const Engine::Order::price_t&>(le64toh(data_[ProtocolPositions_::PRICE])); }
    inline Engine::Order::price_t qty_() const { return static_cast<const Engine::Order::qty_t&>(le64toh(data_[ProtocolPositions_::QTY])); }

    std::array<unsigned char, ProtocolPositions_::PACKAGE_END> data_;
    size_t data_length_ = 0;

};

} /* API */

#endif /* CreateOrderPackage_h */
