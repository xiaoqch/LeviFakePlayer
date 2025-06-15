#include "FakePlayerCommand.h"
#include <ranges>
#include <string>
#include <vector>

#include "lfp/Config.h"
#include "lfp/utils/DebugUtils.h"
#include "ll/api/command/EnumName.h"
#include "magic_enum.hpp"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPosition.h"
#include "mc/server/commands/CommandRegistry.h"
#include "mc/world/Minecraft.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/Dimension.h"

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/Optional.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/PlayerInfo.h"

#include "lfp/api/events/FakePlayerCreatedEvent.h"
#include "lfp/api/events/FakePlayerRemoveEvent.h"
#include "lfp/manager/FakePlayer.h"
#include "lfp/manager/FakePlayerManager.h"
#include "lfp/utils/ColorUtils.h"
#include "lfp/utils/SimulatedPlayerUtils.h"

#define KEY_NO_TARGET       "%commands.generic.noTargetMatch"
#define KEY_TOO_MANY_TARGET "%commands.generic.tooManyTargets"

#define AssertUniqueTarget(results)                                                                \
    if (results.empty()) return output.error(KEY_NO_TARGET);                                       \
    else if (results.count() > 1) return output.error(KEY_TOO_MANY_TARGET);

namespace lfp {

enum class LeviFakePlayerNames;

constexpr std::string_view FakePlayerCommand::mListSoftEnumName =
    ll::command::enum_name_v<LeviFakePlayerNames>;
std::vector<std::string> FakePlayerCommand::mList;


enum LFPCommandAction : int { remove, login, logout, swap, autologin };
struct LFPCommandWithSoftEnum {
    LFPCommandAction                           action = (LFPCommandAction)-1;
    ll::command::SoftEnum<LeviFakePlayerNames> name;
};
struct LFPCreateCommand {
    std::string                            name;
    ll::command::Optional<CommandPosition> pos;
    ll::command::Optional<DimensionType>   dim;
};
struct LFPCommandWithNameStr {
    std::string name;
};

constexpr auto FULL_COMMAND_NAME = "levifakeplayer";

void FakePlayerCommand::initSoftEnum() {
    auto nameView = FakePlayerManager::getManager().getTimeSortedFakePlayerList()
                  | std::views::transform([](auto fp) { return fp->getRealName(); });
    ll::command::CommandRegistrar::getInstance().tryRegisterSoftEnum(
        std::string{mListSoftEnumName},
        std::vector<std::string>(nameView.begin(), nameView.end())
    );
    ASSERT(ll::command::CommandRegistrar::getInstance().hasSoftEnum(std::string{mListSoftEnumName})
    );

    ll::event::EventBus::getInstance().emplaceListener<event::FakePlayerCreatedEvent>([](auto& ev) {
        auto name = ev.name();
        DEBUGW("Add {} to FakePlayer SoftEnum", name);
        ll::command::CommandRegistrar::getInstance().addSoftEnumValues(
            std::string{mListSoftEnumName},
            {name}
        );
    });
    ll::event::EventBus::getInstance().emplaceListener<event::FakePlayerRemoveEvent>([](auto& ev) {
        auto name = ev.name();
        DEBUGW("Remove {} from FakePlayer SoftEnum", name);
        ll::command::CommandRegistrar::getInstance().removeSoftEnumValues(
            std::string{mListSoftEnumName},
            {name}
        );
    });
}


void FakePlayerCommand::setup(LeviFakePlayerConfig::CommandConfig const& config) {
    // Register commands.
    auto commandRegistry = ll::service::getCommandRegistry();
    if (!commandRegistry) {
        throw std::runtime_error("failed to get command registry");
    }
    initSoftEnum();

    auto& command = ll::command::CommandRegistrar::getInstance()
                        .getOrCreateCommand(
                            FULL_COMMAND_NAME,
                            "FakePlayer Plugin For LeviLamina",
                            config.permission
                        )
                        .alias(config.alias);


    // lfp <remove|login|logout|swap|autologin> <name: string>
    command.overload<LFPCommandWithSoftEnum>().required("action").required("name").execute(
        [](CommandOrigin const& origin, CommandOutput& output, LFPCommandWithSoftEnum const& params
        ) {
            auto& manager = FakePlayerManager::getManager();
            auto& name    = params.name;
            auto  fp      = manager.tryGetFakePlayer(name);
            if (!fp) return output.error("FakePlayer \"{}\" not exists", name);

            switch (params.action) {
            case LFPCommandAction::remove: {
                if (!manager.remove(*fp)) output.error("Remove Failed");
                else output.success("Remove Success");
                break;
            }
            case LFPCommandAction::login: {
                if (fp->isOnline()) return output.error("FakePlayer {} already online", name);
                if (!fp->login()) output.error("Unknown Error in FakePlayer {} login", name);
                else output.success("FakePlayer {} login success", name);
                break;
            }
            case LFPCommandAction::logout: {
                if (!fp->isOnline()) return output.error("FakePlayer {} not online", name);
                if (!fp->logout()) output.error("Unknown Error in FakePlayer {} logout", name);
                else output.success("FakePlayer {} logout success", name);
                break;
            }
            case LFPCommandAction::swap: {
                auto pl = origin.getEntity();
                if (!pl->isPlayer()) return output.error("swap action can only execute by player");
                if (!manager.swapContainer(*fp, *static_cast<SimulatedPlayer*>(pl)))
                    output.error("Unknown Error in FakePlayer {} swap bag", name);
                else output.success("FakePlayer {} swap bag success", name);
                break;
            }
            case LFPCommandAction::autologin: {
                if (!manager.switchAutoLogin(*fp)) output.error("Failed to switch auto login");
                else output.success("switch auto login as {} success", fp->shouldAutoLogin());
                break;
            }
            default:
                output.error("Unknown action {}", magic_enum::enum_name(params.action));
            }
        }
    );

    // lfp create <name: string> [pos: x y z] [dim: Dimension]
    command.overload<LFPCreateCommand>()
        .text("create")
        .required("name") // Why?
        .optional("pos")
        .optional("dim")
        .execute([](CommandOrigin const&    origin,
                    CommandOutput&          output,
                    LFPCreateCommand const& params) {
            auto& manager = FakePlayerManager::getManager();
            auto& name    = params.name;
            if (manager.tryGetFakePlayer(name))
                return output.error("FakePlayer {} already exists.");

            FakePlayer* fp;
            if (!params.pos) {
                fp = manager.create(name);
            } else {
                DimensionType dimId = 0;
                if (params.dim) {
                    dimId = *params.dim;
                } else if (auto dim = origin.getDimension()) {
                    dimId = dim->getDimensionId();
                }
                fp = manager.create(name, params.pos->getBlockPos(0, origin, {}), dimId);
            }
            if (!fp) output.error(fmt::format("Failed to create FakePlayer {}.", name));
            output.success(fmt::format("Create FakePlayer {} successfully.", fp->getRealName()));
        });

    // lfp import name
    command.overload<LFPCommandWithNameStr>().text("import").optional("name").execute(
        [](CommandOrigin const&, CommandOutput& output, LFPCommandWithNameStr const& params) {
            auto& manager = FakePlayerManager::getManager();
            auto& name    = params.name;
            if (!name.empty()) {
                if (manager.tryGetFakePlayer(name))
                    return output.error("FakePlayer {} already exists.");
                if (!manager.importData_JavaFakePlayer(name))
                    return output.error("Failed to import FakePlayer {} data", name);
                output.success("FakePlayer {} data import success", name);
                return;
            }
            for (auto& entry : ll::service::PlayerInfo::getInstance().entries()) {
                auto nameUUID = utils::sp_utils::JAVA_nameUUIDFromBytes(entry.name);
                if (entry.uuid == nameUUID && manager.importData_JavaFakePlayer(entry.name)) {
                    output.success("{} - {} - {}", entry.name, entry.xuid, entry.uuid.asString());
                }
                if (output.mSuccessCount == 0)
                    return output.error("Unknown Error in FakePlayer data import", name);
                output.success("{} FakePlayers data import success", output.mSuccessCount);
                output.mSuccessCount -= 1;
            }
            if (output.mSuccessCount <= 0) {
                output.error("No FakePlayer be found in player db");
            }
        }
    );

    // lfp help
    command.overload().text("help").execute([](CommandOrigin const&, CommandOutput& output) {
        output.success("PLUGIN_USAGE");
    });

    // lfp list
    command.overload().text("list").execute([](CommandOrigin const&, CommandOutput& output) {
        auto&       manager    = FakePlayerManager::getManager();
        std::string playerList = "";
        for (auto& fp : manager.iter()) {
            playerList += ", ";
            playerList +=
                fp.isOnline() ? lfp::utils::color_utils::green(fp.getRealName()) : fp.getRealName();
            output.mSuccessCount++;
        }
        playerList.erase(0, 2);
        if (playerList.empty()) output.error(lfp::utils::color_utils::red("No FakePlayer."));
        else output.success(fmt::format("List: {}", playerList));
    });
}


} // namespace lfp