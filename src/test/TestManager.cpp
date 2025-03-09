#include "TestManager.h"
#include <memory>
#include <string_view>

#include <mc/server/DedicatedServer.h>
#include <mc/server/ServerLevel.h>
#include <mc/world/Minecraft.h>

#include "ll/api//coro/CoroTask.h"
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/server/ServerStartedEvent.h"
#include "ll/api/io/LogLevel.h"
#include "ll/api/io/Logger.h"
#include "ll/api/io/LoggerRegistry.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "ll/api/utils/ErrorUtils.h"
#include <ll/api/service/Bedrock.h>


#include "lfp/LeviFakePlayer.h"
#include "lfp/utils/ColorUtils.h"


using namespace ll::literals;

namespace lfp::test {

ll::io::Logger& testLogger = []() -> ll::io::Logger& {
    static auto logger = ll::io::LoggerRegistry::getInstance().getOrCreate("LFPTest");
    return *logger;
}();


ll::coro::CoroTask<bool> TestManager::runAllTest() {
    testLogger.debug("Start testing, {} tests needed to run", mTests.size());
    std::vector<std::string> failureList{};
    for (auto& test : mTests) {
#ifdef TEST_THIS_SET_ONLY
        if (test->mSetName != TEST_THIS_SET_ONLY) continue;
#endif
        testLogger.debug("Runing Test {}", test->fullName());
        try {
            auto res = test->runThis();
            if (res) {
                co_await ll::chrono::ticks{1};
                co_await *res;
            }
        } catch (...) {
            testLogger.error("Test {} failed with throw:", test->fullName());
            ll::error_utils::printCurrentException(testLogger);
        }
        if (!test->isSuccess()) {
            testLogger.error("Test {} run failed: {}", test->fullName(), test->failureMessage());
            failureList.push_back(test->fullName());
        }
    }
    if (failureList.size()) {
        testLogger.error("Failed list: {}", failureList);
    }
    co_return failureList.empty();
}

ll::coro::CoroTask<void> testEntry() {
    lfp::LeviFakePlayer::getLogger().setLevel(ll::io::LogLevel::Info);
    testLogger.setLevel(ll::io::LogLevel::Debug);
#if defined(TEST_THIS_SET_ONLY)
    testLogger.warn(
        "Testing \"{}\" Test Set Only!!!, check src/test/TestManager.h for details",
        TEST_THIS_SET_ONLY
    );
#elif defined(NDEBUG)
    testLogger.warn("Testing in Release Mode");
#endif
    co_await 10_tick;
    auto result = co_await TestManager::getInstance().runAllTest();
    if (!result) {
        testLogger.error("Test Failed!!!");
        co_await (5 * 20_tick);
    } else {
        testLogger.info(lfp::utils::color_utils::convertToConsole(
            lfp::utils::color_utils::green("All Tests Passed!")
        ));
    }
#if defined(NDEBUG) && !defined(TEST_THIS_SET_ONLY)
    testLogger.info("Request server shutdown by LeviFakePlayer in Test Mode");
    if (result) ll::service::getMinecraft()->mApp.requestServerShutdown("");
    else std::exit(EXIT_FAILURE);
#endif
    co_return;
}

[[maybe_unused]] bool testStarted = []() {
    auto& eventBus = ll::event::EventBus::getInstance();
    auto  listener = eventBus.emplaceListener<ll::event::server::ServerStartedEvent>([](auto) {
        testEntry().launch(ll::thread::ServerThreadExecutor::getDefault());
        testStarted = true;
    });
    return false;
}();


LFP_TEST(TestManagerTest, Sync) { return; }
LFP_CO_TEST(TestManagerTest, Async) { co_return; }

} // namespace lfp::test