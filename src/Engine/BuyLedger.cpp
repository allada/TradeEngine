#include "BuyLedger.h"

#include <Judy.h>
#include <queue>
#include "Trade.h"

using namespace Engine;

// Highest price.
static Order::price_t tipPrice_ = 0;
static uint64_t count_ = 0;
static uint64_t processedCount_ = 0;
static Pvoid_t PJLArray = (Pvoid_t) NULL;

typedef std::queue<Order*> QueueOrders;

std::unique_ptr<Order> BuyLedger::tipOrder()
{
    QueueOrders* orderQueue;
    Word_t index = -1;
    {
        QueueOrders** orderQueuePtr = reinterpret_cast<QueueOrders**>(JudyLLast(PJLArray, &index, PJE0));
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
        tipPrice_ = 0;
    } else if (orderQueue->size() <= 1) {
        EXPECT_GT(count_, 0);
        Word_t newTipIndex = index;
        QueueOrders** newTipOrder = reinterpret_cast<QueueOrders**>(JudyLPrev(PJLArray, &newTipIndex, PJE0));
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
    DEBUG("Buy Order Removed {qty: %llu, price: %llu, newTip: %llu, newCount: %llu}", returnOrder->qty(), returnOrder->price(), tipPrice_, count_);
    return returnOrder;
}

Order::price_t BuyLedger::tipPrice()
{
    return tipPrice_;
}

uint64_t BuyLedger::count()
{
    return count_;
}

uint64_t BuyLedger::processedCount()
{
    return processedCount_;
}

void BuyLedger::addOrder(std::unique_ptr<Order> order)
{
    ++processedCount_;
    Order::price_t buyPrice = order->price();
    Order::price_t lastSellPrice = SellLedger::tipPrice();
    if (lastSellPrice <= buyPrice && SellLedger::count() > 0) {
        std::unique_ptr<Order> sellOrder = SellLedger::tipOrder();
        EXPECT_EQ(lastSellPrice, sellOrder->price());
        Trade::execute(std::move(order), std::move(sellOrder), Order::OrderType::BUY);
        return;
    }

    if (buyPrice > tipPrice_) {
        tipPrice_ = buyPrice;
    }
    QueueOrders** pointer = reinterpret_cast<QueueOrders**>(JudyLGet(PJLArray, buyPrice, PJE0));
    // TODO Not sure if i'm sold on using error code to tell if the item item exists.
    if (pointer == NULL) {
        pointer = reinterpret_cast<QueueOrders**>(JudyLIns(&PJLArray, buyPrice, PJE0));
        EXPECT_NE(pointer, PJERR);
        *pointer = new QueueOrders;
    }
    DEBUG("Buy Order Added {qty: %llu, price: %llu}", order->qty(), order->price());
    (*pointer)->push(order.release());
    ++count_;
}

void BuyLedger::reset()
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
