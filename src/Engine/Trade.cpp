#include "Trade.h"

#include <vector>

using namespace Engine;

static std::vector<std::unique_ptr<TradeDeligate>> tradesDeligates_;

void Trade::execute(std::unique_ptr<Order> buyOrder, std::unique_ptr<Order> sellOrder, Order::OrderType taker)
{
    EXPECT_NE(buyOrder->qty(), 0);
    EXPECT_NE(sellOrder->qty(), 0);
    Order::qty_t buyQty = buyOrder->qty();
    Order::qty_t sellQty = sellOrder->qty();
    Order::price_t buyPrice = buyOrder->price();
    Order::price_t sellPrice = sellOrder->price();
    std::shared_ptr<Trade> trade = WrapUnique(new Trade(std::move(buyOrder), std::move(sellOrder), taker));
    EXPECT_NE(trade->qty_, 0);

    Trade::broadcast_(trade);

    if (trade->qty_ != buyQty) {
        EXPECT_GT(buyQty, trade->qty_);
        BuyLedger::addOrder(WrapUnique(new Order(buyPrice, buyQty - trade->qty_, Order::OrderType::BUY)));
    } else if (trade->qty_ != sellQty) {
        EXPECT_GT(sellQty, trade->qty_);
        SellLedger::addOrder(WrapUnique(new Order(sellPrice, sellQty - trade->qty_, Order::OrderType::SELL)));
    }
}

void Trade::addDeligate(std::unique_ptr<TradeDeligate> deligate)
{
    tradesDeligates_.push_back(std::move(deligate));
}

void Trade::removeDeligatesForTest()
{
    tradesDeligates_.clear();
}

void Trade::broadcast_(std::shared_ptr<Trade> trade)
{
    for (auto const& it: tradesDeligates_) {
        it->tradeExecuted(trade);
    }
}