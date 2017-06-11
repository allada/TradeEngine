#include "SellLedger.h"

#include <Judy.h>
#include <queue>
#include "Trade.h"
#include "BuyLedger.h"

using namespace Engine;

// Lowest price.
static Order::price_t tipPrice_ = std::numeric_limits<Order::price_t>::max();
static Pvoid_t PJLArray = (Pvoid_t) NULL;

SellLedger* SellLedger::instance_ = nullptr;

typedef std::queue<Order*> QueueOrders;

std::unique_ptr<Order> SellLedger::tipOrder()
{
    EXPECT_UI_THREAD();
    QueueOrders* orderQueue;
    Word_t index = 0;
    {
        QueueOrders** orderQueuePtr = reinterpret_cast<QueueOrders**>(JudyLFirst(PJLArray, &index, PJE0));
        EXPECT_NE(orderQueuePtr, PJERR);

        if (orderQueuePtr == NULL) {
            tipPrice_ = 0;
            return nullptr;
        }

        // Because delete events happen below Judy arrays may reallocate everything so we deref here.
        orderQueue = *orderQueuePtr;
        --count_;
    }
    if (count_ == 0) {
        EXPECT_EQ(JudyLCount(PJLArray, 0, -1, PJE0), 1);
        tipPrice_ = std::numeric_limits<Order::price_t>::max();
    } else if (orderQueue->size() <= 1) {
        EXPECT_GT(count_, 0);
        Word_t newTipIndex = index;
        QueueOrders** newTipOrder = reinterpret_cast<QueueOrders**>(JudyLNext(PJLArray, &newTipIndex, PJE0));
        EXPECT_NE(newTipOrder, nullptr);
        EXPECT_NE(*newTipOrder, nullptr);
        EXPECT_NE(*newTipOrder, PJERR);
        tipPrice_ = (*newTipOrder)->front()->price();
    }

    Order* order = orderQueue->front();
    orderQueue->pop();
    std::unique_ptr<Order> returnOrder = WrapUnique(order);
    if (orderQueue->size() <= 0) {
        delete orderQueue;
        int return_code;
        JLD(return_code, PJLArray, index);
        EXPECT_EQ(return_code, 1);
    }
    // DEBUG("Sell Order Removed {qty: %llu, price: %llu, newTip: %llu}", returnOrder->qty(), returnOrder->price(), tipPrice_);
    return returnOrder;
}

Order::price_t SellLedger::tipPrice()
{
    EXPECT_UI_THREAD();
    return tipPrice_;
}

uint64_t SellLedger::count()
{
    EXPECT_UI_THREAD();
    return count_;
}

void SellLedger::handleOrder(std::unique_ptr<Order> order)
{
    EXPECT_UI_THREAD();
    EXPECT_NE(deligate_.get(), nullptr);
    deligate_->orderReceived(order);
    addOrder(std::move(order));
}

void SellLedger::addOrder(std::unique_ptr<Order> order)
{
    EXPECT_UI_THREAD();
    Order::price_t sellPrice = order->price();
    Order::price_t lastBuyPrice = BuyLedger::instance()->tipPrice();
    if (lastBuyPrice >= sellPrice && BuyLedger::instance()->count() > 0) {
        std::unique_ptr<Order> buyOrder = BuyLedger::instance()->tipOrder();
        EXPECT_EQ(lastBuyPrice, buyOrder->price());
        Trade::execute(std::move(buyOrder), std::move(order), Order::side_t::SELL);
        return;
    }

    if (sellPrice < tipPrice_) {
        tipPrice_ = sellPrice;
    }
    QueueOrders** pointer = reinterpret_cast<QueueOrders**>(JudyLGet(PJLArray, sellPrice, PJE0));
    // TODO Not sure if i'm sold on using error code to tell if the item item exists.
    if (pointer == NULL) {
        pointer = reinterpret_cast<QueueOrders**>(JudyLIns(&PJLArray, sellPrice, PJE0));
        EXPECT_NE(pointer, PJERR);
        *pointer = new QueueOrders;
    }
    // DEBUG("Sell Order Added {qty: %16llu, price: %16llu}", order->qty(), order->price());
    Order* ledgerOrder = order.release();
    (*pointer)->push(ledgerOrder);
    ++count_;

    EXPECT_NE(deligate_.get(), nullptr);
    deligate_->addedToLedger(const_cast<Order &>(*ledgerOrder));
}

void SellLedger::reset()
{
    EXPECT_UI_THREAD();
    size_t i = 0;
    QueueOrders** orderQueueRef = reinterpret_cast<QueueOrders**>(JudyLFirst(PJLArray, &i, PJE0));
    while (orderQueueRef != NULL) {
        QueueOrders* orderQueue = *orderQueueRef;
        EXPECT_FALSE(orderQueue->empty());
        while (!orderQueue->empty()) {
            Order* order = orderQueue->front();
            orderQueue->pop();
            delete order;
            --count_;
        }
        delete orderQueue;
        orderQueueRef = reinterpret_cast<QueueOrders**>(JudyLNext(PJLArray, &i, PJE0));
    }
    JudyLFreeArray(&PJLArray, PJE0);
    PJLArray = (Pvoid_t) NULL;
    deligate_ = nullptr;
    EXPECT_EQ(count_, 0);
}
