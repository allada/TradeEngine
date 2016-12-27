#ifndef SellLedger_h
#define SellLedger_h

#include "Order.h"
#include "Ledger.h"
#include "../Common.h"

namespace Engine {

class SellLedger : public Ledger {
public:
    static SellLedger* instance()
    {
        if (!instance_)
            instance_ = new SellLedger;
        return instance_;
    }

    std::unique_ptr<Order> tipOrder() override;
    Order::price_t tipPrice() override;
    uint64_t count() override;
    void reset() override;

    void addOrder(std::unique_ptr<Order>) override;
    void handleOrder(std::unique_ptr<Order>) override;

private:
    SellLedger() { }
    size_t count_ = 0;
    static SellLedger* instance_;

};

} /* Engine */

#endif /* SellLedger_h */
