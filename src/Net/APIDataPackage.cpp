#include "APIDataPackage.h"

#include "../Engine/Order.h"
#include "../Engine/SellLedger.h"
#include "../Engine/BuyLedger.h"
#include "../Thread/ThreadManager.h"
#include "../Thread/TaskQueueThread.h"

std::shared_ptr<Thread::TaskQueueThread> uiThread;

using namespace Net;

class CreateOrderTask : public virtual Thread::Tasker {
public:
    CreateOrderTask(Engine::Order::price_t price, Engine::Order::qty_t qty, Engine::Order::OrderType type)
        : order_(WrapUnique(new Engine::Order(price, qty, type))) { }

    void run() override {
        if (type_ == Engine::Order::OrderType::BUY) {
            Engine::BuyLedger::addOrder(std::move(order_));
        } else {
            Engine::SellLedger::addOrder(std::move(order_));
        }
    }

private:
    std::unique_ptr<Engine::Order> order_;
    Engine::Order::OrderType type_;

};

std::unique_ptr<Thread::Tasker> APIDataPackage::makeTask() {
    EXPECT_TRUE(is_done_);
    EXPECT_EQ(data_length_, PACKAGE_SIZE);

    if (version_() != PROTOCOL_VERSION) {
        WARNING("Got version %d but we only support version %d", version_(), PROTOCOL_VERSION);
        return nullptr;
    }

    const uint32_t checksum = computeChecksum_();
    if (checksum != checksum_()) {
        WARNING("Got bad checksum, ignoring packet");
        return nullptr;
    }

    switch (action_()) {
        case ACTION_CREATE_BUY_ORDER:
            //uint64_t orderId = extractOrderId();
            uiThread->addTask(WrapUnique(new CreateOrderTask(price_(), qty_(), Engine::Order::OrderType::BUY)));
            break;
        case ACTION_CREATE_SELL_ORDER:
            uiThread->addTask(WrapUnique(new CreateOrderTask(price_(), qty_(), Engine::Order::OrderType::SELL)));
            break;
        default:
            WARNING("Unknown action. Dropping packet.");
    }






    return nullptr;
}
