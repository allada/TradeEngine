#ifndef Trade_h
#define Trade_h

#include <algorithm>
#include <vector>
#include "Order.h"

namespace Engine {

class Trade;

class TradeDeligate {
public:
    virtual ~TradeDeligate() { };
    virtual void tradeExecuted(std::shared_ptr<Trade>) = 0;

};

class Trade {
    FAST_ALLOCATE(Trade)
public:
    static void execute(std::unique_ptr<Order> buyOrder, std::unique_ptr<Order> sellOrder, Order::OrderType taker);
    static void addDeligate(std::unique_ptr<TradeDeligate> deligate) { tradesDeligates_.push_back(std::move(deligate)); }
    static void removeDeligatesForTest() { tradesDeligates_.clear(); }

    Order::qty_t qty() const { return qty_; }
    Order::price_t price() const { return price_; }
    Order::OrderType taker() const { return taker_; }
    const std::unique_ptr<Order>& buyOrder() const { return buy_order_; }
    const std::unique_ptr<Order>& sellOrder() const { return sell_order_; }

private:
    Trade(std::unique_ptr<Order> buyOrder, std::unique_ptr<Order> sellOrder, Order::OrderType taker)
        : buy_order_(std::move(buyOrder))
        , sell_order_(std::move(sellOrder))
        , taker_(taker)
    {
        price_ = taker == Order::OrderType::SELL ? buy_order_->price() : sell_order_->price();
        qty_ = std::min(buy_order_->qty(), sell_order_->qty());
    }

    static void broadcast_(std::shared_ptr<Trade> trade)
    {
        for (auto const& it: Trade::tradesDeligates_) {
            it->tradeExecuted(trade);
        }
    }

    std::unique_ptr<Order> buy_order_;
    std::unique_ptr<Order> sell_order_;
    Order::OrderType taker_;

    Order::price_t price_;
    Order::qty_t qty_;

    static std::vector<std::unique_ptr<TradeDeligate>> tradesDeligates_;

};

} /* Engine */

#endif /* Trade_h */
