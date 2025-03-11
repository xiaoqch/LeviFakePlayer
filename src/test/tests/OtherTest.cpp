#include "../TestManager.h"
#include "../utils/TestUtils.h"
#include "test/utils/CoroUtils.h"

#include "magic_enum.hpp"
#include "mc/server/commands/CommandRegistry.h"
#include <string>

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"


using namespace ll::chrono_literals;

namespace lfp::test {

LL_TYPE_INSTANCE_HOOK(
    HOOK_CommandRegistry_registerCommand1,
    ll::memory::HookPriority::Normal,
    CommandRegistry,
    &CommandRegistry::registerCommand,
    void,
    ::std::string const&     name,
    char const*              description,
    ::CommandPermissionLevel requirement,
    ::CommandFlag            f1,
    ::CommandFlag            f2
) {
    fmt::println(
        "CommandRegistry::registerCommand({}, {}, {}, {}, {})",
        name,
        description,
        magic_enum::enum_name(requirement),
        magic_enum::enum_name(f1.value),
        magic_enum::enum_name(f2.value)
    );
    return origin(
        std::forward<::std::string const&>(name),
        std::forward<char const*>(description),
        std::forward<::CommandPermissionLevel>(requirement),
        std::forward<::CommandFlag>(f1),
        std::forward<::CommandFlag>(f2)
    );
};

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