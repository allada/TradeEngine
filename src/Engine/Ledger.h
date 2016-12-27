#ifndef Ledger_h
#define Ledger_h

namespace Engine {

class LedgerDeligate;

class Ledger {
public:
    virtual std::unique_ptr<Order> tipOrder() = 0;
    virtual Order::price_t tipPrice() = 0;
    virtual uint64_t count() = 0;
    virtual void reset() = 0;

    virtual void addOrder(std::unique_ptr<Order>) = 0;
    virtual void handleOrder(std::unique_ptr<Order>) = 0;

    void setDeligate(std::unique_ptr<LedgerDeligate> deligate)
    {
        deligate_ = std::move(deligate);
    }

protected:
    std::unique_ptr<LedgerDeligate> deligate_;

};

class LedgerDeligate {
public:
    virtual void addedToLedger(const Order&) = 0;
    virtual void orderReceived(const std::unique_ptr<Order>&) = 0;

};

} /* Engine */

#endif /* Ledger_h */
