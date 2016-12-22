#include "SellLedger.h"

#include <Judy.h>
#include <queue>
#include "Trade.h"

using namespace Engine;

// Lowest price.
static Order::price_t tipPrice_ = std::numeric_limits<Order::price_t>::max();
static uint64_t count_ = 0;
static uint64_t processedCount_ = 0;
static Pvoid_t PJLArray = (Pvoid_t) NULL;

typedef std::queue<Order*> QueueOrders;

std::unique_ptr<Order> SellLedger::tipOrder()
{
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
    DEBUG("Sell Order Removed {qty: %llu, price: %llu, newTip: %llu}", returnOrder->qty(), returnOrder->price(), tipPrice_);
    return returnOrder;
}

Order::price_t SellLedger::tipPrice()
{
    return tipPrice_;
}

uint64_t SellLedger::count()
{
    return count_;
}

uint64_t SellLedger::processedCount()
{
    return processedCount_;
}

void SellLedger::addOrder(std::unique_ptr<Order> order)
{
    ++processedCount_;
    Order::price_t sellPrice = order->price();
    Order::price_t lastBuyPrice = BuyLedger::tipPrice();
    if (lastBuyPrice >= sellPrice && BuyLedger::count() > 0) {
        std::unique_ptr<Order> buyOrder = BuyLedger::tipOrder();
        EXPECT_EQ(lastBuyPrice, buyOrder->price());
        Trade::execute(std::move(buyOrder), std::move(order), Order::OrderType::SELL);
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
    DEBUG("Sell Order Added {qty: %llu, price: %llu}", order->qty(), order->price());
    (*pointer)->push(order.release());
    ++count_;
}

void SellLedger::reset()
{
    size_t i = 0;
    QueueOrders** orderQueue = reinterpret_cast<QueueOrders**>(JudyLFirst(PJLArray, &i, PJE0));
    while (orderQueue != NULL) {
        while (!(*orderQueue)->empty()) {
            Order* order = (*orderQueue)->front();
            (*orderQueue)->pop();
            delete order;
        }
        delete *orderQueue;
        //JudyLDel(&PJLArray, i);
        orderQueue = reinterpret_cast<QueueOrders**>(JudyLNext(PJLArray, &i, PJE0));
    }
    JudyLFreeArray(&PJLArray, PJE0);
}
