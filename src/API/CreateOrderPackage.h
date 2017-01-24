#ifndef CreateOrderPackage_h
#define CreateOrderPackage_h

#include "DataPackageType.h"
#include "../Engine/Order.h"
#include "../Engine/CreateOrderTask.h"
#include "../Common.h"
#include <type_traits>

namespace API {

class CreateOrderPackage : public virtual DataPackageType {
    FAST_ALLOCATE(CreateOrderPackage)
private:
    struct NibblesPerPosition_ {
        static constexpr int ACTION_TYPE  = 2;
        static constexpr int FLAGS        = 2;
        static constexpr int SIDE         = 1;
        static constexpr int ORDER_TYPE   = 1;
        static constexpr int UNUSED       = 2;
        static constexpr int ORDER_ID     = 32;
        static constexpr int PRICE        = 16;
        static constexpr int QTY          = 16;
    };

    struct ProtocolPositions_ {
        static constexpr int ACTION_TYPE  = 0; // +1
        static constexpr int FLAGS        = 1;  // +1
        static constexpr int SIDE         = 2;  // Note this is bitshifted >> 4.
        static constexpr int ORDER_TYPE   = 2;  // Note this is masked 0xF.
        static constexpr int UNUSED       = 3;  // +1
        static constexpr int ORDER_ID     = 4;  // +16
        static constexpr int PRICE        = 20; // +8
        static constexpr int QTY          = 28; // +8
        // If this is modified remember to update PACKAGE_SIZE to biggest value + 1.
    };

    enum Flags_ {
        POST_ONLY = 0x1,
    };

    enum class order_side_t_ : uint8_t {
        BUY  = 0,
        SELL = 1,
    };

    enum class order_type_t_ : uint8_t {
        LIMIT  = 0,
        MARKET = 1,
        // STOP   = 2,
    };


    static order_side_t_ sideFromOrder_(const Engine::Order& order)
    {
        return order.side() == Engine::Order::side_t::BUY ? order_side_t_::BUY : order_side_t_::SELL;
    }

    static order_type_t_ typeFromOrder_(const Engine::Order& order)
    {
        switch (order.type()) {
            case Engine::Order::type_t::LIMIT:
                return order_type_t_::LIMIT;
            case Engine::Order::type_t::MARKET:
                return order_type_t_::MARKET;
            // case Engine::Order::type_t::STOP:
            //     return order_type_t_::STOP;
        }
    }

public:
    static constexpr int PACKAGE_SIZE = 36;

    CreateOrderPackage() { }
    CreateOrderPackage(const Engine::Order& order)
    {
        auto                       side    = static_cast<std::underlying_type<order_side_t_>::type>(sideFromOrder_(order));
        auto                       type    = static_cast<std::underlying_type<order_type_t_>::type>(typeFromOrder_(order));
        uint8_t                    flags   = order.isPostOnly() ? POST_ONLY : 0;
        Engine::Order::order_id_t  orderId = order.id();
        uint64_t                   price   = htole64(order.price());
        uint64_t                   qty     = htole64(order.qty());

        uint8_t sideAndType = (side << 4) | (type & 0xF);

        memcpy(&data_[ProtocolPositions_::SIDE],     &sideAndType, (NibblesPerPosition_::SIDE + NibblesPerPosition_::ORDER_TYPE) / 2);
        // No need to put OrderType here because it's done above.
        memcpy(&data_[ProtocolPositions_::FLAGS],    &flags,       NibblesPerPosition_::FLAGS / 2);
        memcpy(&data_[ProtocolPositions_::ORDER_ID], &orderId,     NibblesPerPosition_::ORDER_ID / 2);
        memcpy(&data_[ProtocolPositions_::PRICE],    &price,       NibblesPerPosition_::PRICE / 2);
        memcpy(&data_[ProtocolPositions_::QTY],      &qty,         NibblesPerPosition_::QTY / 2);
    }

    bool setData(const unsigned char* data, size_t len)
    {
        if (UNLIKELY(len < PACKAGE_SIZE)) {
            WARNING("CreateOrderPackage size not large enough, got %lu but must be at least %lu", len, PACKAGE_SIZE);
            return false;
        }
        memcpy(data_.begin(), data, PACKAGE_SIZE);
        return true;
    }

    std::unique_ptr<Engine::Order> makeOrder() const
    {
        Engine::Order::order_id_t orderId = reinterpret_cast<const Engine::Order::order_id_t&>(data_[ProtocolPositions_::ORDER_ID]);
        Engine::Order::price_t    price   = price_();
        Engine::Order::qty_t      qty     = qty_();
        Engine::Order::type_t     type    = type_();
        Engine::Order::side_t     side    = side_();
        
        Engine::Order::flags_t    flags   = flags_();

        return WrapUnique(new Engine::Order(orderId, price, qty, side, type, flags));
    }

    std::unique_ptr<Threading::Tasker> makeTask() override
    {
        return WrapUnique(new Engine::CreateOrderTask(makeOrder()));
    }

private:
    Engine::Order::order_id_t orderId_() const {  return reinterpret_cast<const Engine::Order::order_id_t&>(data_[ProtocolPositions_::ORDER_ID]); }
    Engine::Order::price_t price_() const { return static_cast<const Engine::Order::price_t>(le64toh(*reinterpret_cast<const uint64_t*>(data_.cbegin() + ProtocolPositions_::PRICE))); }
    Engine::Order::qty_t qty_() const { return static_cast<const Engine::Order::qty_t>(le64toh(*reinterpret_cast<const uint64_t*>(data_.cbegin() + ProtocolPositions_::QTY))); }

    Engine::Order::side_t side_() const {
        auto side = static_cast<typename std::underlying_type<order_side_t_>::type>(data_[ProtocolPositions_::SIDE]) >> 4;
        switch (reinterpret_cast<const order_side_t_&>(side)) {
            case order_side_t_::BUY:
                return Engine::Order::side_t::BUY;
            case order_side_t_::SELL:
                return Engine::Order::side_t::SELL;
        }
        WARNING("ERROR side is invalid %lu", static_cast<uint8_t>(side));
        // TODO Maybe do something better?
        return Engine::Order::side_t::BUY;
    }

    Engine::Order::type_t type_() const {
        auto type = static_cast<typename std::underlying_type<order_type_t_>::type>(data_[ProtocolPositions_::ORDER_TYPE]) & 0xF;
        switch (reinterpret_cast<const order_type_t_&>(type)) {
            case order_type_t_::MARKET:
                return Engine::Order::type_t::MARKET;
            case order_type_t_::LIMIT:
                return Engine::Order::type_t::LIMIT;
            // case order_type_t_::STOP:
            //     return Engine::Order::type_::STOP;
        }
        WARNING("ERROR type is invalid %lu", static_cast<uint8_t>(type));
        // TODO Maybe do something better?
        return Engine::Order::type_t::MARKET;
    }

    Engine::Order::flags_t flags_() const {
        uint8_t flags = data_[ProtocolPositions_::FLAGS];
        Engine::Order::flags_t outFlags = 0;
        if (flags & POST_ONLY) {
            outFlags |= Engine::Order::Flags::POST_ONLY;
        }
        return outFlags;
    }

    std::array<unsigned char, CreateOrderPackage::PACKAGE_SIZE> data_;

};

} /* API */

#endif /* CreateOrderPackage_h */
