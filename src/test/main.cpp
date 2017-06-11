#include "gtest/gtest.h"

#include "Threading/MainThread.h"
#include "Threading/ThreadManager.h"
#include "Engine/BuyLedger.h"
#include "Engine/SellLedger.h"
#include "Engine/Trade.h"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::shared_ptr<Threading::MainThread> mainThread;
    mainThread = std::make_shared<Threading::MainThread>();
    Threading::ThreadManager::setMainThread(mainThread);

    int result = RUN_ALL_TESTS();
    Engine::SellLedger::instance()->reset();
    Engine::BuyLedger::instance()->reset();
    Engine::Trade::removeDeligatesForTest();
    return result;
}