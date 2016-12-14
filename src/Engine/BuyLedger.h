#ifndef BuyLedger_h
#define BuyLedger_h

#include "Order.h"
#include "../Common.h"

namespace Engine {

class BuyLedger {
    STATIC_ONLY(BuyLedger)
public:
    static std::unique_ptr<Order> tipOrder();
    static Order::price_t tipPrice();
    static uint64_t count();

    static void addOrder(std::unique_ptr<Order>);
};

} /* Engine */

#endif /* BuyLedger_h */
