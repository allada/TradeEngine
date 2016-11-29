#ifndef SellLedger_h
#define SellLedger_h

#include "../Common.h"

namespace Engine {

class SellLedger {
    STATIC_ONLY(SellLedger)
public:
    static std::unique_ptr<Order> tipOrder();
    static Order::price_t tipPrice();

    static void addOrder(std::unique_ptr<Order>);
};

} /* Engine */

#endif /* SellLedger_h */
