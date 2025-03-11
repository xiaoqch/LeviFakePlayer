#include "TestUtils.h"

#include "mc/deps/core/utility/MCRESULT.h"
#include "mc/scripting/commands/ScriptCommandOrigin.h"
#include "mc/server/commands/CommandContext.h"
#include "mc/server/commands/CommandVersion.h"
#include "mc/server/commands/MinecraftCommands.h"
#include "mc/world/Minecraft.h"

#include "ll/api/service/Bedrock.h"
#include "ll/api/utils/StringUtils.h"

#include "lfp/command/TickingCommand.h"
#include "lfp/utils/NbtUtils.h"
#include <string>

namespace lfp::test {

namespace {
constexpr auto PrintTickingDetail = false;
}; // namespace


bool executeCommand(
    std::string const&                          command,
    ::std::function<void(int, ::std::string&&)> output,
    DimensionType                               dim
) {
    auto context = CommandContext(
        command,
        std::make_unique<ScriptCommandOrigin>(
            ll::service::getLevel()->asServer(),
            ll::service::getLevel()->getOrCreateDimension(dim).lock().get(),
            output,
            CommandPermissionLevel::Owner
        ),
        CommandVersion::CurrentVersion()
    );
    return ll::service::getMinecraft()->mCommands->executeCommand(context, false).mSuccess;
}

CommandOutputResult executeCommandEx(std::string const& command, DimensionType dim) {
    CommandOutputResult commandResult;
    auto                context = CommandContext(
        command,
        std::make_unique<ScriptCommandOrigin>(
            ll::service::getLevel()->asServer(),
            ll::service::getLevel()->getOrCreateDimension(dim).lock().get(),
            [&](int count, ::std::string&& output) {
                commandResult.successCount = count;
                commandResult.output       = output;
            },
            CommandPermissionLevel::Owner
        ),
        CommandVersion::CurrentVersion()
    );
    auto result           = ll::service::getMinecraft()->mCommands->executeCommand(context, false);
    commandResult.success = result.mSuccess;
    return commandResult;
}

int getTickingChunkCount(BlockSource const& region, BlockPos const& bpos, int range, bool print) {
    auto [info, count] = lfp::genTickingInfo(region, bpos, range);
    if (PrintTickingDetail || print) {
        testLogger.debug(
            "pos: {}, dim: {}, \n{}",
            bpos.toString(),
            (int)region.getDimensionId(),
            ll::utils::string_utils::replaceMcToAnsiCode(info)
        );
    }
    return count;
}

bool logIfNbtChange(CompoundTag const& oldTag, CompoundTag const& newTag) {
    auto [oldDiff, newDiff] = lfp::utils::nbt_utils::tagDiff(oldTag, newTag, 1);
    if (oldDiff && newDiff && oldDiff->size() + newDiff->size() > 0) {
        testLogger.info(
            "nbt data changed after logout and login:\nbefore: {}\nafter:{}",
            oldDiff->toSnbt(SnbtFormat::PrettyConsolePrint),
            newDiff->toSnbt(SnbtFormat::PrettyConsolePrint)
        );
        return true;
    }
    return false;
}

} // namespace lfp::test