#ifndef Trade_h
#define Trade_h

#include <algorithm>

namespace Engine {

class Trade {
public:
    static std::unique_ptr<Order> execute(std::unique_ptr<Order> buyOrder, std::unique_ptr<Order> sellOrder, Order::OrderType taker)
    {
        if (taker == Order::OrderType::SELL) {
            price = buyOrder->price();
        } else {
            price = sellOrder->price();
        }
        new Trade(std::move(buyOrder), std::move(sellOrder), taker);
    }

private:
    Trade(std::unique_ptr<Order> buyOrder, std::unique_ptr<Order> sellOrder, Order::OrderType taker)
        : buy_order_(buyOrder)
        , sell_order_(sellOrder)
        , taker_(taker) { }

    Order buy_order_;
    Order sell_order_;
    Order::OrderType taker_;

};

} /* Engine */

#endif /* Trade_h */
