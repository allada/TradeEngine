#include "gtest/gtest.h"

#include "Engine/BuyLedger.h"
#include "Engine/SellLedger.h"
#include "Engine/Order.h"
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

    void tradeExecuted(std::shared_ptr<Trade> trade) override
    {
        ledgerTest_->tradeExecuted(trade);
    }

    void addedToLedger(const Order&) { }
    void orderReceived(const std::unique_ptr<Order>&) { }

private:
    LedgerTest* ledgerTest_;

};

class MockLedgerDeligate : public virtual LedgerDeligate {
    void addedToLedger(const Order&) { }
    void orderReceived(const std::unique_ptr<Order>&) { }
};

void LedgerTest::SetUp()
{
    EXPECT_EQ(BuyLedger::instance()->count(), 0);
    EXPECT_EQ(SellLedger::instance()->count(), 0);
    Engine::SellLedger::instance()->setDeligate(WrapUnique(new MockLedgerDeligate));
    Engine::BuyLedger::instance()->setDeligate(WrapUnique(new MockLedgerDeligate));
    Trade::addDeligate(WrapUnique(new MockTradeDeligate(this)));
}

void LedgerTest::TearDown()
{
    EXPECT_EQ(BuyLedger::instance()->count(), 0);
    EXPECT_EQ(SellLedger::instance()->count(), 0);
    Trade::removeDeligatesForTest();
    trades_.clear();
}

#define EXPECT_ORDER(_order, _price, _qty, _side) \
    EXPECT_EQ(_order->price(), _price); \
    EXPECT_EQ(_order->qty(), _qty); \
    EXPECT_EQ(_order->side(), _side);

#define EXPECT_ORDER_UNIQUE(_order, _price, _qty, _side) \
    do { \
        const std::unique_ptr<Order> __order = _order; \
        EXPECT_ORDER(__order, _price, _qty, _side); \
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
    const unsigned char cid[] = {'h', 'e', 'l', 'l', 'o', 'f', '0', 'o', 'b', 'a', 'r', '1', '2', '3', '4', '5'};
    const Order::order_id_t id(cid[0]);
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 6;
    const Order::qty_t qty = 3;

    EXPECT_EQ(BuyLedger::instance()->count(), 0);
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, buyPrice, qty, Order::side_t::BUY, Order::type_t::LIMIT)));
    EXPECT_EQ(BuyLedger::instance()->count(), 1);
    std::unique_ptr<Order> buyOrder = BuyLedger::instance()->tipOrder();
    EXPECT_ORDER(buyOrder, buyPrice, qty, Order::side_t::BUY);

    EXPECT_EQ(SellLedger::instance()->count(), 0);
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, sellPrice, qty, Order::side_t::SELL, Order::type_t::LIMIT)));
    EXPECT_EQ(SellLedger::instance()->count(), 1);
    std::unique_ptr<Order> sellOrder = SellLedger::instance()->tipOrder();
    EXPECT_ORDER(sellOrder, sellPrice, qty, Order::side_t::SELL);
}

TEST_F(LedgerTest, OrderExecutesOnAdd) {
    const unsigned char cid[] = {'h', 'e', 'l', 'l', 'o', 'f', '0', 'o', 'b', 'a', 'r', '1', '2', '3', '4', '5'};
    const Order::order_id_t id(cid[0]);
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 5;
    const Order::qty_t qty = 3;
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, buyPrice, qty, Order::side_t::BUY, Order::type_t::LIMIT)));
    EXPECT_EQ(BuyLedger::instance()->count(), 1);
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, sellPrice, qty, Order::side_t::SELL, Order::type_t::LIMIT)));
    BuyLedger::instance()->tipOrder(); // Remove the orders just added
    SellLedger::instance()->tipOrder();
}

TEST_F(LedgerTest, SellOrderExecutesAndStaysIfLargerThanCurrentQtyAvailable) {
    const unsigned char cid[] = {'h', 'e', 'l', 'l', 'o', 'f', '0', 'o', 'b', 'a', 'r', '1', '2', '3', '4', '5'};
    const Order::order_id_t id(cid[0]);
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 5;
    const Order::qty_t buyQty = 5;
    const Order::qty_t sellQty = 7;
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, buyPrice, buyQty, Order::side_t::BUY, Order::type_t::LIMIT)));
    EXPECT_EQ(BuyLedger::instance()->count(), 1);
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, sellPrice, sellQty, Order::side_t::SELL, Order::type_t::LIMIT)));
    EXPECT_EQ(SellLedger::instance()->count(), 1);
    EXPECT_EQ(BuyLedger::instance()->count(), 0);
    std::unique_ptr<Order> sellOrder = SellLedger::instance()->tipOrder();
    EXPECT_ORDER(sellOrder, sellPrice, sellQty - buyQty, Order::side_t::SELL);
}

TEST_F(LedgerTest, BuyOrderExecutesAndStaysIfLargerThanCurrentQtyAvailable) {
    const unsigned char cid[] = {'h', 'e', 'l', 'l', 'o', 'f', '0', 'o', 'b', 'a', 'r', '1', '2', '3', '4', '5'};
    const Order::order_id_t id(cid[0]);
    const Order::price_t buyPrice = 5;
    const Order::price_t sellPrice = 5;
    const Order::qty_t buyQty = 10;
    const Order::qty_t sellQty = 7;
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, sellPrice, sellQty, Order::side_t::SELL, Order::type_t::LIMIT)));
    EXPECT_EQ(SellLedger::instance()->count(), 1);
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, buyPrice, buyQty, Order::side_t::BUY, Order::type_t::LIMIT)));
    EXPECT_EQ(BuyLedger::instance()->count(), 1);
    EXPECT_EQ(SellLedger::instance()->count(), 0);
    std::unique_ptr<Order> buyOrder = BuyLedger::instance()->tipOrder();
    EXPECT_ORDER(buyOrder, buyPrice, buyQty - sellQty, Order::side_t::BUY);
}

TEST_F(LedgerTest, EnsureSingleTradeBroadcast) {
    const unsigned char cid[] = {'h', 'e', 'l', 'l', 'o', 'f', '0', 'o', 'b', 'a', 'r', '1', '2', '3', '4', '5'};
    const Order::order_id_t id(cid[0]);
    const Order::price_t buyPrice = 7;
    const Order::price_t sellPrice = 5;
    const Order::qty_t buyQty = 10;
    const Order::qty_t sellQty = 7;
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, sellPrice, sellQty, Order::side_t::SELL, Order::type_t::LIMIT)));
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, buyPrice, buyQty, Order::side_t::BUY, Order::type_t::LIMIT)));
    std::unique_ptr<Order> buyOrder = BuyLedger::instance()->tipOrder();
    EXPECT_EQ(this->trades_.size(), 1);

    const std::shared_ptr<Trade>& trade = this->trades_.back();
    EXPECT_TRADE(trade, sellPrice, sellQty, Order::side_t::BUY);
    EXPECT_ORDER(trade->buyOrder(), buyPrice, buyQty, Order::side_t::BUY);
    EXPECT_ORDER(trade->sellOrder(), sellPrice, sellQty, Order::side_t::SELL);
}

TEST_F(LedgerTest, BuyOrderToConsumeMultipleOrdersAndStaysOnBooks) {
    const unsigned char cid[] = {'h', 'e', 'l', 'l', 'o', 'f', '0', 'o', 'b', 'a', 'r', '1', '2', '3', '4', '5'};
    const Order::order_id_t id(cid[0]);
    const Order::price_t sellPrice1 = 5;
    const Order::price_t sellPrice2 = 5;
    const Order::qty_t sellQty1 = 4;
    const Order::qty_t sellQty2 = 7;
    const Order::price_t buyPrice = 7;
    const Order::qty_t buyQty = 12;
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, sellPrice1, sellQty1, Order::side_t::SELL, Order::type_t::LIMIT)));
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, sellPrice2, sellQty2, Order::side_t::SELL, Order::type_t::LIMIT)));
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, buyPrice, buyQty, Order::side_t::BUY, Order::type_t::LIMIT)));
    EXPECT_EQ(this->trades_.size(), 2);

    const std::shared_ptr<Trade>& trade = this->trades_[0];
    
    EXPECT_TRADE(trade, sellPrice1, sellQty1, Order::side_t::BUY);
    EXPECT_ORDER(trade->buyOrder(), buyPrice, buyQty, Order::side_t::BUY);
    EXPECT_ORDER(trade->sellOrder(), sellPrice1, sellQty1, Order::side_t::SELL);

    const std::shared_ptr<Trade>& trade2 = this->trades_[1];
    EXPECT_TRADE(trade2, sellPrice2, sellQty2, Order::side_t::BUY);
    EXPECT_ORDER(trade2->buyOrder(), buyPrice, buyQty - trade->qty(), Order::side_t::BUY);
    EXPECT_ORDER(trade2->sellOrder(), sellPrice2, sellQty2, Order::side_t::SELL);
    
    const std::unique_ptr<Order>& buyOrder = BuyLedger::instance()->tipOrder();
    EXPECT_ORDER(buyOrder, buyPrice, buyQty - sellQty1 - sellQty2, Order::side_t::BUY);
}

TEST_F(LedgerTest, TestSmallOrderBatch) {
    const unsigned char cid[] = {'h', 'e', 'l', 'l', 'o', 'f', '0', 'o', 'b', 'a', 'r', '1', '2', '3', '4', '5'};
    const Order::order_id_t id(cid[0]);
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, 40, 107, Order::side_t::SELL, Order::type_t::LIMIT)));
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, 9, 8, Order::side_t::BUY, Order::type_t::LIMIT)));
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, 2, 5, Order::side_t::SELL, Order::type_t::LIMIT)));
        POP_AND_CHECK_TRADE(9, 5, Order::side_t::SELL);
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, 1, 6, Order::side_t::BUY, Order::type_t::LIMIT)));
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, 6, 9, Order::side_t::SELL, Order::type_t::LIMIT)));
        POP_AND_CHECK_TRADE(9, 3, Order::side_t::SELL);
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, 5, 10, Order::side_t::BUY, Order::type_t::LIMIT)));
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, 8, 7, Order::side_t::SELL, Order::type_t::LIMIT)));
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, 7, 8, Order::side_t::BUY, Order::type_t::LIMIT)));
        POP_AND_CHECK_TRADE(6, 6, Order::side_t::BUY);
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, 2, 5, Order::side_t::SELL, Order::type_t::LIMIT)));
        POP_AND_CHECK_TRADE(5, 3, Order::side_t::SELL);
        POP_AND_CHECK_TRADE(7, 2, Order::side_t::SELL);
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, 1, 6, Order::side_t::BUY, Order::type_t::LIMIT)));
    SellLedger::instance()->addOrder(WrapUnique(new Order(id, 4, 3, Order::side_t::SELL, Order::type_t::LIMIT)));
        POP_AND_CHECK_TRADE(5, 3, Order::side_t::SELL);
    BuyLedger::instance()->addOrder(WrapUnique(new Order(id, 3, 4, Order::side_t::BUY, Order::type_t::LIMIT)));

    EXPECT_EQ(this->trades_.size(), 0);

    EXPECT_EQ(SellLedger::instance()->count(), 2);
    std::unique_ptr<Order> order;
    EXPECT_ORDER_UNIQUE(SellLedger::instance()->tipOrder(), 8, 7, Order::side_t::SELL);
    EXPECT_ORDER_UNIQUE(SellLedger::instance()->tipOrder(), 40, 107, Order::side_t::SELL);

    EXPECT_EQ(BuyLedger::instance()->count(), 4);
    EXPECT_ORDER_UNIQUE(BuyLedger::instance()->tipOrder(), 5, 4, Order::side_t::BUY);
    EXPECT_ORDER_UNIQUE(BuyLedger::instance()->tipOrder(), 3, 4, Order::side_t::BUY);
    EXPECT_ORDER_UNIQUE(BuyLedger::instance()->tipOrder(), 1, 6, Order::side_t::BUY);
    EXPECT_ORDER_UNIQUE(BuyLedger::instance()->tipOrder(), 1, 6, Order::side_t::BUY);
}

} // namespace
