#ifndef TradeDeligate_h
#define TradeDeligate_h

namespace Engine {

class Trade;

class TradeDeligate {
public:
    virtual void tradeExecuted(std::shared_ptr<Trade>) const = 0;

};

} /* Engine */

#endif /* TradeDeligate_h */
