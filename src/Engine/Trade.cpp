#include "Trade.h"

#include "BuyLedger.h"
#include "SellLedger.h"

using namespace Engine;

std::vector<std::unique_ptr<TradeDeligate>> Trade::tradesDeligates_;

void Trade::execute(std::unique_ptr<Order> buyOrder, std::unique_ptr<Order> sellOrder, Order::OrderType taker)
{
    EXPECT_NE(buyOrder->qty(), 0);
    EXPECT_NE(sellOrder->qty(), 0);
    Order::order_id_t buyId = buyOrder->id();
    Order::order_id_t sellId = sellOrder->id();
    Order::qty_t buyQty = buyOrder->qty();
    Order::qty_t sellQty = sellOrder->qty();
    Order::price_t buyPrice = buyOrder->price();
    Order::price_t sellPrice = sellOrder->price();
    std::shared_ptr<Trade> trade = WrapUnique(new Trade(std::move(buyOrder), std::move(sellOrder), taker));
    EXPECT_NE(trade->qty_, 0);

    Trade::broadcast_(trade);

    if (trade->qty_ != buyQty) {
        EXPECT_GT(buyQty, trade->qty_);
        BuyLedger::instance()->addOrder(WrapUnique(new Order(buyId, buyPrice, buyQty - trade->qty_, Order::OrderType::BUY)));
    } else if (trade->qty_ != sellQty) {
        EXPECT_GT(sellQty, trade->qty_);
        SellLedger::instance()->addOrder(WrapUnique(new Order(sellId, sellPrice, sellQty - trade->qty_, Order::OrderType::SELL)));
    }
}