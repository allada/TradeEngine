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

#define EXPECT_ORDER(_order, _price, _qty, _type) \
    EXPECT_EQ(_order->price(), _price); \
    EXPECT_EQ(_order->qty(), _qty); \
    EXPECT_EQ(_order->type(), _type);

#define EXPECT_ORDER_UNIQUE(_order, _price, _qty, _type) \
    do { \
        const std::unique_ptr<Order> __order = _order; \
        EXPECT_ORDER(__order, _price, _qty, _type); \
    } while(false);

#define EXPECT_TRADE(_trade, _price, _qty, _taker) \
    EXPECT_EQ(_trade->price(), _price); \
    EXPECT_EQ(_trade->qty(), _qty); \
    EXPECT_EQ(_trade->taker(), _taker);

#define POP_AND_CHECK_TRADE(_price, _qty, _taker) \
    do { \
        EXPECT_GT(this->trades_.size(), 0); \
        if (!this->trades_.size()) \
            break; \
        const std::shared_ptr<Trade> _trade = this->trades_.back(); \
        this->trades_.pop_back(); \
        EXPECT_TRADE(_trade, _price, _qty, _taker); \
    } while(false);

TEST_F(LedgerTest, AddOrder) {
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 6;
    const Order::qty_t qty = 3;

    EXPECT_EQ(BuyLedger::count(), 0);
    BuyLedger::addOrder(WrapUnique(new Order(buyPrice, qty, Order::OrderType::BUY)));
    EXPECT_EQ(BuyLedger::count(), 1);
    std::unique_ptr<Order> buyOrder = BuyLedger::tipOrder();
    EXPECT_ORDER(buyOrder, buyPrice, qty, Order::OrderType::BUY);

    EXPECT_EQ(SellLedger::count(), 0);
    SellLedger::addOrder(WrapUnique(new Order(sellPrice, qty, Order::OrderType::SELL)));
    EXPECT_EQ(SellLedger::count(), 1);
    std::unique_ptr<Order> sellOrder = SellLedger::tipOrder();
    EXPECT_ORDER(sellOrder, sellPrice, qty, Order::OrderType::SELL);
}

TEST_F(LedgerTest, OrderExecutesOnAdd) {
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 5;
    const Order::qty_t qty = 3;
    BuyLedger::addOrder(WrapUnique(new Order(buyPrice, qty, Order::OrderType::BUY)));
    EXPECT_EQ(BuyLedger::count(), 1);
    SellLedger::addOrder(WrapUnique(new Order(sellPrice, qty, Order::OrderType::SELL)));
    BuyLedger::tipOrder(); // Remove the orders just added
    SellLedger::tipOrder();
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
    EXPECT_ORDER(sellOrder, sellPrice, sellQty - buyQty, Order::OrderType::SELL);
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
    EXPECT_ORDER(buyOrder, buyPrice, buyQty - sellQty, Order::OrderType::BUY);
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
    EXPECT_TRADE(trade, sellPrice, sellQty, Order::OrderType::BUY);
    EXPECT_ORDER(trade->buyOrder(), buyPrice, buyQty, Order::OrderType::BUY);
    EXPECT_ORDER(trade->sellOrder(), sellPrice, sellQty, Order::OrderType::SELL);
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
    
    EXPECT_TRADE(trade, sellPrice1, sellQty1, Order::OrderType::BUY);
    EXPECT_ORDER(trade->buyOrder(), buyPrice, buyQty, Order::OrderType::BUY);
    EXPECT_ORDER(trade->sellOrder(), sellPrice1, sellQty1, Order::OrderType::SELL);

    const std::shared_ptr<Trade>& trade2 = this->trades_[1];
    EXPECT_TRADE(trade2, sellPrice2, sellQty2, Order::OrderType::BUY);
    EXPECT_ORDER(trade2->buyOrder(), buyPrice, buyQty - trade->qty(), Order::OrderType::BUY);
    EXPECT_ORDER(trade2->sellOrder(), sellPrice2, sellQty2, Order::OrderType::SELL);
    
    const std::unique_ptr<Order>& buyOrder = BuyLedger::tipOrder();
    EXPECT_ORDER(buyOrder, buyPrice, buyQty - sellQty1 - sellQty2, Order::OrderType::BUY);
}

TEST_F(LedgerTest, TestSmallOrderBatch) {
    SellLedger::addOrder(WrapUnique(new Order(40, 107, Order::OrderType::SELL)));
    BuyLedger::addOrder(WrapUnique(new Order(9, 8, Order::OrderType::BUY)));
    SellLedger::addOrder(WrapUnique(new Order(2, 5, Order::OrderType::SELL)));
        POP_AND_CHECK_TRADE(9, 5, Order::OrderType::SELL);
    BuyLedger::addOrder(WrapUnique(new Order(1, 6, Order::OrderType::BUY)));
    SellLedger::addOrder(WrapUnique(new Order(6, 9, Order::OrderType::SELL)));
        POP_AND_CHECK_TRADE(9, 3, Order::OrderType::SELL);
    BuyLedger::addOrder(WrapUnique(new Order(5, 10, Order::OrderType::BUY)));
    SellLedger::addOrder(WrapUnique(new Order(8, 7, Order::OrderType::SELL)));
    BuyLedger::addOrder(WrapUnique(new Order(7, 8, Order::OrderType::BUY)));
        POP_AND_CHECK_TRADE(6, 6, Order::OrderType::BUY);
    SellLedger::addOrder(WrapUnique(new Order(2, 5, Order::OrderType::SELL)));
        POP_AND_CHECK_TRADE(5, 3, Order::OrderType::SELL);
        POP_AND_CHECK_TRADE(7, 2, Order::OrderType::SELL);
    BuyLedger::addOrder(WrapUnique(new Order(1, 6, Order::OrderType::BUY)));
    SellLedger::addOrder(WrapUnique(new Order(4, 3, Order::OrderType::SELL)));
        POP_AND_CHECK_TRADE(5, 3, Order::OrderType::SELL);
    BuyLedger::addOrder(WrapUnique(new Order(3, 4, Order::OrderType::BUY)));

    EXPECT_EQ(this->trades_.size(), 0);

    EXPECT_EQ(SellLedger::count(), 2);
    std::unique_ptr<Order> order;
    EXPECT_ORDER_UNIQUE(SellLedger::tipOrder(), 8, 7, Order::OrderType::SELL);
    EXPECT_ORDER_UNIQUE(SellLedger::tipOrder(), 40, 107, Order::OrderType::SELL);

    EXPECT_EQ(BuyLedger::count(), 4);
    EXPECT_ORDER_UNIQUE(BuyLedger::tipOrder(), 5, 4, Order::OrderType::BUY);
    EXPECT_ORDER_UNIQUE(BuyLedger::tipOrder(), 3, 4, Order::OrderType::BUY);
    EXPECT_ORDER_UNIQUE(BuyLedger::tipOrder(), 1, 6, Order::OrderType::BUY);
    EXPECT_ORDER_UNIQUE(BuyLedger::tipOrder(), 1, 6, Order::OrderType::BUY);
}

} // namespace
