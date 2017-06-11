#ifndef BuyLedger_h
#define BuyLedger_h

#include "Order.h"
#include "Ledger.h"
#include "../Common.h"

namespace Engine {

class BuyLedger : public Ledger {
public:
    static BuyLedger* instance()
    {
        if (!instance_)
            instance_ = new BuyLedger;
        return instance_;
    }

    std::unique_ptr<Order> tipOrder() override;
    Order::price_t tipPrice() override;
    uint64_t count() override;
    void reset() override;

    void addOrder(std::unique_ptr<Order>) override;
    void handleOrder(std::unique_ptr<Order>) override;

private:
    BuyLedger() { }
    size_t count_ = 0;
    static BuyLedger* instance_;

};

} /* Engine */

#endif /* BuyLedger_h */
