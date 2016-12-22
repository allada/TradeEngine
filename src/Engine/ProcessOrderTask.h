#ifndef ProcessOrderTask_h
#define ProcessOrderTask_h

#include "Order.h"
#include "SellLedger.h"
#include "BuyLedger.h"
#include "../API/DataPackage.h"
#include "../Threading/Tasker.h"
#include "../Common.h"

namespace Engine {

class ProcessOrderTask : public virtual Threading::Tasker {
public:
    ProcessOrderTask(std::unique_ptr<API::DataPackage> package)
        : package_(std::move(package)) { }

    void run() override {
        EXPECT_NE(package_.get(), nullptr);
        EXPECT_EQ(package_->version(), API::DataPackage::PROTOCOL_VERSION);

        if (UNLIKELY(package_->verifyChecksum())) {
            WARNING("Got bad checksum, ignoring packet");
            return;
        }

        switch (package_->action()) {
            case API::DataPackage::CREATE_BUY_ORDER:
                //uint64_t orderId = extractOrderId();
                Engine::BuyLedger::addOrder(WrapUnique(new Order(package_->price(), package_->qty(), Engine::Order::OrderType::BUY)));
                break;
            case API::DataPackage::CREATE_SELL_ORDER:
                Engine::SellLedger::addOrder(WrapUnique(new Order(package_->price(), package_->qty(), Engine::Order::OrderType::SELL)));
                break;
            default:
                WARNING("Unknown action. Dropping order.");
        }
    }

private:
    std::unique_ptr<API::DataPackage> package_;

};

}

#endif /* ProcessOrderTask_h */
