#include "SellLedger.h"
#include "Judy.h"

using namespace Engine;

// Highest price.
Order::price_t tipPrice_ = 0;
Pvoid_t PJLArray = (Pvoid_t) NULL;

std::unique_ptr<Order> SellLedger::tipOrder()
{
    Word_t index = 0;
    Order* order = nullptr;

    JLF(0, PJLArray, index);
    if (index != NULL) {
        int return_code;
        JLD(return_code, PJLArray, index);
        ASSERT_NE(return_code, 0, "Judy delete seemed to fail");
        return WrapUnique(order);
    }
    return nullptr;
}

Order::price_t SellLedger::tipPrice()
{
    return tipPrice_;
}

void SellLedger::addOrder(std::unique_ptr<Order> order)
{
    Order::price_t sellPrice = order->price();
    Order::price_t lastBuyPrice = BuyLedger::tipPrice();
    while (lastBuyPrice >= sellPrice) {
        std::unique_ptr<Order> buyOrder = BuyLedger::tipOrder();
        ASSERT_EQ(lastBuyPrice, buyOrder->price(), "Between retreiving the tip order and executing it the price changed!");
        order = Trade::execute(std::move(buyOrder), std::move(order), Order::OrderType::SELL);
        // All done, order was completely satisfied.
        if (order == nullptr)
            return;
        lastBuyPrice = BuyLedger::tipPrice();
    }

    if (price > tipPrice_) {
        tipPrice_ = price;
    }
    int return_code;
    Word_t* pointer;
    JLI(pointer, PJLArray, price);
    ASSERT_NE(pointer, PJERR, "Judy insert seems to have failed");
    *pointer = order.release();
}