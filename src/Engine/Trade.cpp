#include "Trade.h"

#include "BuyLedger.h"
#include "SellLedger.h"

using namespace Engine;

std::vector<std::unique_ptr<TradeDeligate>> Trade::tradesDeligates_;

void Trade::execute(std::unique_ptr<Order> buyOrder, std::unique_ptr<Order> sellOrder, Order::side_t taker)
{
    EXPECT_UI_THREAD();
    EXPECT_NE(buyOrder->qty(), 0);
    EXPECT_NE(sellOrder->qty(), 0);
    Order::qty_t buyQty = buyOrder->qty();
    Order::qty_t sellQty = sellOrder->qty();
    std::shared_ptr<Trade> trade = WrapUnique(new Trade(std::move(buyOrder), std::move(sellOrder), taker));
    EXPECT_NE(trade->qty_, 0);

    Trade::broadcast_(trade);

    if (trade->qty_ != buyQty) {
        EXPECT_GT(buyQty, trade->qty_);
        BuyLedger::instance()->addOrder(WrapUnique(new Order(trade->sellOrder()->id(), trade->buyOrder()->price(), buyQty - trade->qty_, Order::side_t::BUY, trade->buyOrder()->type())));
    } else if (trade->qty_ != sellQty) {
        EXPECT_GT(sellQty, trade->qty_);
        SellLedger::instance()->addOrder(WrapUnique(new Order(trade->sellOrder()->id(), trade->sellOrder()->price(), sellQty - trade->qty_, Order::side_t::SELL, trade->sellOrder()->type())));
    }
}