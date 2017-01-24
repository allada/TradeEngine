#ifndef CreateOrderTask_h
#define CreateOrderTask_h

#include "Order.h"
#include "SellLedger.h"
#include "BuyLedger.h"
#include "../API/DataPackage.h"
#include "../Threading/Tasker.h"
#include "../Common.h"

namespace Engine {

class CreateOrderTask : public virtual Threading::Tasker {
    FAST_ALLOCATE(CreateOrderTask)
public:
    CreateOrderTask(std::unique_ptr<Order> order)
        : order_(std::move(order))
    {
        EXPECT_IO_THREAD();
    }

    void run() override {
        EXPECT_UI_THREAD();
        if (order_->side() == Engine::Order::side_t::BUY) {
            Engine::BuyLedger::instance()->handleOrder(std::move(order_));
        } else {
            Engine::SellLedger::instance()->handleOrder(std::move(order_));
        }
    }

private:
    std::unique_ptr<Order> order_;

};

}

#endif /* CreateOrderTask_h */
