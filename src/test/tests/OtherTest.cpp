#include "../TestManager.h"
#include "../utils/TestUtils.h"
#include "test/utils/CoroUtils.h"
#include <string>

#include "ll/api/chrono/GameChrono.h"

using namespace ll::chrono_literals;

namespace lfp::test {

LFP_CO_TEST(OtherTest, BdsSaveCommand) {
    auto result                    = executeCommandEx("save hold");
    auto& [success, count, output] = result;
    testLogger.debug("success: {}, count: {}, output: {}", success, count, output);
    EXPECT_TRUE(success);

    result.success = false;
    co_await waitUntil(
        [&] {
            result = executeCommandEx("save query");
            return result.success && result.output.starts_with("Saving...");
        },
        10 * 20_tick
    );

    EXPECT_TRUE(result.success == true);
    EXPECT_TRUE(!result.output.empty());
    EXPECT_TRUE(executeCommandEx("save resume").success == true);
    co_return;
}

} // namespace lfp::test