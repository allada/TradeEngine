#include "gtest/gtest.h"

#include "Engine/BuyLedger.h"
#include "Engine/SellLedger.h"
#include "Engine/Order.h"
#include "Engine/TradeDeligate.h"
#include "Engine/Trade.h"
#include "Common.h"

namespace {
using namespace Engine;

class LedgerTest : public ::testing::Test {
public:
    void tradeExecuted(std::shared_ptr<Trade> trade)
    {
        trades_.push_back(trade);
    }

protected:
    virtual void SetUp();
    virtual void TearDown();
    std::vector<std::shared_ptr<Trade>> trades_;

};

class MockTradeDeligate : public virtual TradeDeligate {
public:
    MockTradeDeligate(LedgerTest* ledgerTest)
        : ledgerTest_(ledgerTest) { }

    void tradeExecuted(std::shared_ptr<Trade> trade) const
    {
        ledgerTest_->tradeExecuted(trade);
    }

private:
    LedgerTest* ledgerTest_;

};

void LedgerTest::SetUp()
{
    EXPECT_EQ(BuyLedger::count(), 0);
    EXPECT_EQ(SellLedger::count(), 0);
    Trade::addDeligate(WrapUnique(new MockTradeDeligate(this)));
}

void LedgerTest::TearDown()
{
    EXPECT_EQ(BuyLedger::count(), 0);
    EXPECT_EQ(SellLedger::count(), 0);
    Trade::removeDeligatesForTest();
    trades_.clear();
}

TEST_F(LedgerTest, AddOrder) {
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 6;
    const Order::qty_t qty = 3;

    EXPECT_EQ(BuyLedger::count(), 0);
    BuyLedger::addOrder(WrapUnique(new Order(buyPrice, qty, Order::OrderType::BUY)));
    EXPECT_EQ(BuyLedger::count(), 1);
    std::unique_ptr<Order> buyOrder = BuyLedger::tipOrder();
    EXPECT_EQ(buyOrder->price(), buyPrice);
    EXPECT_EQ(buyOrder->qty(), qty);
    EXPECT_EQ(buyOrder->type(), Order::OrderType::BUY);

    EXPECT_EQ(SellLedger::count(), 0);
    SellLedger::addOrder(WrapUnique(new Order(sellPrice, qty, Order::OrderType::SELL)));
    EXPECT_EQ(SellLedger::count(), 1);
    std::unique_ptr<Order> sellOrder = SellLedger::tipOrder();
    EXPECT_EQ(sellOrder->price(), sellPrice);
    EXPECT_EQ(sellOrder->qty(), qty);
    EXPECT_EQ(sellOrder->type(), Order::OrderType::SELL);
}

TEST_F(LedgerTest, OrderExecutesOnAdd) {
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 5;
    const Order::qty_t qty = 3;
    BuyLedger::addOrder(WrapUnique(new Order(buyPrice, qty, Order::OrderType::BUY)));
    EXPECT_EQ(BuyLedger::count(), 1);
    SellLedger::addOrder(WrapUnique(new Order(sellPrice, qty, Order::OrderType::SELL)));
}

TEST_F(LedgerTest, SellOrderExecutesAndStaysIfLargerThanCurrentQtyAvailable) {
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 5;
    const Order::qty_t buyQty = 5;
    const Order::qty_t sellQty = 7;
    BuyLedger::addOrder(WrapUnique(new Order(buyPrice, buyQty, Order::OrderType::BUY)));
    EXPECT_EQ(BuyLedger::count(), 1);
    SellLedger::addOrder(WrapUnique(new Order(sellPrice, sellQty, Order::OrderType::SELL)));
    EXPECT_EQ(SellLedger::count(), 1);
    EXPECT_EQ(BuyLedger::count(), 0);
    std::unique_ptr<Order> sellOrder = SellLedger::tipOrder();
    EXPECT_EQ(sellOrder->qty(), sellQty - buyQty);
}

TEST_F(LedgerTest, BuyOrderExecutesAndStaysIfLargerThanCurrentQtyAvailable) {
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 5;
    const Order::qty_t buyQty = 10;
    const Order::qty_t sellQty = 7;
    SellLedger::addOrder(WrapUnique(new Order(sellPrice, sellQty, Order::OrderType::SELL)));
    EXPECT_EQ(SellLedger::count(), 1);
    BuyLedger::addOrder(WrapUnique(new Order(buyPrice, buyQty, Order::OrderType::BUY)));
    EXPECT_EQ(BuyLedger::count(), 1);
    EXPECT_EQ(SellLedger::count(), 0);
    std::unique_ptr<Order> buyOrder = BuyLedger::tipOrder();
    EXPECT_EQ(buyOrder->qty(), buyQty - sellQty);
}

TEST_F(LedgerTest, EnsureSingleTradeBroadcast) {
    const Order::price_t buyPrice = 7;
    const Order::price_t sellPrice = 5;
    const Order::qty_t buyQty = 10;
    const Order::qty_t sellQty = 7;
    SellLedger::addOrder(WrapUnique(new Order(sellPrice, sellQty, Order::OrderType::SELL)));
    BuyLedger::addOrder(WrapUnique(new Order(buyPrice, buyQty, Order::OrderType::BUY)));
    std::unique_ptr<Order> buyOrder = BuyLedger::tipOrder();
    EXPECT_EQ(this->trades_.size(), 1);

    const std::shared_ptr<Trade>& trade = this->trades_.back();
    EXPECT_EQ(trade->qty(), sellQty);
    EXPECT_EQ(trade->price(), sellPrice);
    EXPECT_EQ(trade->taker(), Order::OrderType::BUY);

    EXPECT_EQ(trade->buyOrder()->price(), buyPrice);
    EXPECT_EQ(trade->buyOrder()->qty(), buyQty);
    EXPECT_EQ(trade->buyOrder()->type(), Order::OrderType::BUY);

    EXPECT_EQ(trade->sellOrder()->price(), sellPrice);
    EXPECT_EQ(trade->sellOrder()->qty(), sellQty);
    EXPECT_EQ(trade->sellOrder()->type(), Order::OrderType::SELL);

}

TEST_F(LedgerTest, BuyOrderToConsumeMultipleOrdersAndStaysOnBooks) {
    const Order::price_t sellPrice1 = 5;
    const Order::price_t sellPrice2 = 5;
    const Order::qty_t sellQty1 = 4;
    const Order::qty_t sellQty2 = 7;
    const Order::price_t buyPrice = 7;
    const Order::qty_t buyQty = 12;
    SellLedger::addOrder(WrapUnique(new Order(sellPrice1, sellQty1, Order::OrderType::SELL)));
    SellLedger::addOrder(WrapUnique(new Order(sellPrice2, sellQty2, Order::OrderType::SELL)));
    BuyLedger::addOrder(WrapUnique(new Order(buyPrice, buyQty, Order::OrderType::BUY)));
    EXPECT_EQ(this->trades_.size(), 2);

    const std::shared_ptr<Trade>& trade = this->trades_[0];
    
    EXPECT_EQ(trade->qty(), sellQty1);
    EXPECT_EQ(trade->price(), sellPrice1);
    EXPECT_EQ(trade->taker(), Order::OrderType::BUY);

    EXPECT_EQ(trade->buyOrder()->price(), buyPrice);
    EXPECT_EQ(trade->buyOrder()->qty(), buyQty);
    EXPECT_EQ(trade->buyOrder()->type(), Order::OrderType::BUY);

    EXPECT_EQ(trade->sellOrder()->price(), sellPrice1);
    EXPECT_EQ(trade->sellOrder()->qty(), sellQty1);
    EXPECT_EQ(trade->sellOrder()->type(), Order::OrderType::SELL);

    const std::shared_ptr<Trade>& trade2 = this->trades_[1];
    EXPECT_EQ(trade2->qty(), sellQty2);
    EXPECT_EQ(trade2->price(), sellPrice2);
    EXPECT_EQ(trade2->taker(), Order::OrderType::BUY);

    EXPECT_EQ(trade2->buyOrder()->price(), buyPrice);
    EXPECT_EQ(trade2->buyOrder()->qty(), buyQty - trade->qty());
    EXPECT_EQ(trade2->buyOrder()->type(), Order::OrderType::BUY);

    EXPECT_EQ(trade2->sellOrder()->price(), sellPrice2);
    EXPECT_EQ(trade2->sellOrder()->qty(), sellQty2);
    EXPECT_EQ(trade2->sellOrder()->type(), Order::OrderType::SELL);

    const std::unique_ptr<Order>& buyOrder = BuyLedger::tipOrder();
    EXPECT_EQ(buyOrder->qty(), buyQty - sellQty1 - sellQty2);
    EXPECT_EQ(buyOrder->price(), buyPrice);
    EXPECT_EQ(buyOrder->type(), Order::OrderType::BUY);

}

} // namespace
