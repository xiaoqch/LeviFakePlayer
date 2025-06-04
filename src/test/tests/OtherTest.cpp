#include "../TestManager.h"
#include "../utils/TestUtils.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/runtime/RuntimeCommand.h"
#include "ll/api/command/runtime/RuntimeOverload.h"
#include "mc/server/commands/CommandOutput.h"
#include "test/utils/CoroUtils.h"

#include "magic_enum.hpp"
#include "mc/server/commands/CommandRegistry.h"
#include "mc/server/commands/standard/ListDCommand.h"

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"


using namespace ll::chrono_literals;

namespace lfp::test {


LFP_CO_TEST(OtherTest, BdsSaveCommand) {
    co_await waitUntil([] {
        return ll::service::getCommandRegistry()
            .transform([](CommandRegistry& reg) {
                return nullptr != reg.findCommand("levifakeplayer");
            })
            .value_or(false);
    });
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