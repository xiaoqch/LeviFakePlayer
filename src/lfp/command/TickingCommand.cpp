
#include "./TickingCommand.h"

#include "mc/deps/core/utility/AutomaticID.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandPosition.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/service/Bedrock.h"

#include "lfp/utils/ColorUtils.h"

namespace lfp {

#define KEY_NO_TARGET       "%commands.generic.noTargetMatch"
#define KEY_TOO_MANY_TARGET "%commands.generic.tooManyTargets"
constexpr const int DEFAULT_RANGE = 10;

std::pair<std::string, int>
genTickingInfo(BlockSource const& region, BlockPos const& bpos, int range) {
    int  chunk_x = bpos.x >> 4;
    int  chunk_z = bpos.z >> 4;
    auto max_cx  = chunk_x + range;
    auto min_cx  = chunk_x - range;
    auto max_cz  = chunk_z + range;
    auto min_cz  = chunk_z - range;
    // auto               totalChunksCount = (max_cx - min_cx + 1) * (max_cz - min_cz + 1);
    auto               tickimgChunkCount = 0;
    static std::string centerLabel       = lfp::utils::color_utils::green("Ｘ");
    static std::string untickingCenter   = "Ｘ";
    static std::string tickingLabel      = lfp::utils::color_utils::green("＃");
    static std::string inRegionLabel     = "Ｏ";
    static std::string outofRegionLabel  = "＋";
    static std::string unloadedLabel     = lfp::utils::color_utils::dark_gray("－");
    std::ostringstream loadInfo;
    std::set<__int64>  playerChunkHashs;
    auto               level = ll::service::getLevel();
    for (auto actor : level->getRuntimeActorList()) {
        if (!actor->isPlayer()) {
            continue;
        }
        // auto player = ll::service::getLevel()->getPlayer(uuid);
        auto player = static_cast<Player*>(actor);
        if (player->getDimensionId() == region.getDimensionId()) {
            playerChunkHashs.insert(ChunkPos(player->getPosition()).hash());
        }
    }
    auto& mainBlockSource = region.getDimensionConst().getBlockSourceFromMainChunkSource();
    for (auto cx = min_cx; cx <= max_cx; ++cx) {
        loadInfo << " \n";
        for (auto cz = min_cz; cz <= max_cz; ++cz) {
            auto        chunkHash = ChunkPos(cx, cz).hash();
            std::string label;
            auto        chunk   = region.getChunk({cx, cz});
            auto        ticking = nullptr != chunk
                        && chunk->mLastTick->tickID + 1 >= level->getCurrentServerTick().tickID;
            if (ticking) ++tickimgChunkCount;
            if (cx == chunk_x && cz == chunk_z) {
                label = ticking ? centerLabel : untickingCenter;
            } else {
                if (chunk) {
                    label = ticking ? tickingLabel : inRegionLabel;
                } else {
                    chunk = mainBlockSource.getChunk({cx, cz});
                    if (chunk) label = outofRegionLabel;
                    else label = unloadedLabel;
                }
            }
            if (ticking && playerChunkHashs.find(chunkHash) != playerChunkHashs.end()) {
                // ColorFormat::removeColorCode(label);
                label = lfp::utils::color_utils::red_bold(label);
            }
            loadInfo << label;
        }
    }
    return {loadInfo.str(), tickimgChunkCount};
}

bool processCommand(
    class CommandOrigin const&,
    class CommandOutput& output,
    BlockSource const&   region,
    BlockPos const&      bpos,
    int                  range
) {
    auto loadInfo = genTickingInfo(region, bpos, range);
    output.success(fmt::format(
        "Dimension: {}, pos: {}, Center ChunkPos: {}, Info: {}",
        (int)region.getDimensionId(),
        bpos.toString(),
        ChunkPos(bpos).toString(),
        loadInfo.first
    ));
    return true;
}

bool processCommand(
    class CommandOrigin const& origin,
    class CommandOutput&       output,
    Actor*                     actor,
    int                        range
) {
    auto bpos   = actor->getFeetBlockPos();
    auto region = &actor->getDimensionBlockSource();
    if (actor->isSimulatedPlayer()) region = &((SimulatedPlayer*)actor)->_getRegion();
    return processCommand(origin, output, *region, bpos, range);
}

struct TickingTargetParam {
    CommandSelector<Player> target;
    int                     range = DEFAULT_RANGE;
};

struct TickingPosParam {
    CommandPosition pos;
    DimensionType   dimension = -1;
    int             range     = DEFAULT_RANGE;
};

struct TickingRangeParam {
    int range = DEFAULT_RANGE;
};

void TickingCommand::setup() {

    auto commandRegistry = ll::service::getCommandRegistry();
    if (!commandRegistry) {
        throw std::runtime_error("failed to get command registry");
    }

    auto& command = ll::command::CommandRegistrar::getInstance().getOrCreateCommand(
        "ticking",
        "Show Ticking chunks",
        CommandPermissionLevel::Any
    );
#ifdef LFP_DEBUG
    command.alias("t");
#endif
    // ticking as player [range]
    command.overload<TickingTargetParam>().text("as").required("target").optional("range").execute(
        [](class CommandOrigin const& origin,
           class CommandOutput&       output,
           TickingTargetParam const&  params) {
            auto result = params.target.results(origin);
            if (result.empty()) {
                output.error(KEY_NO_TARGET);
            } else if (result.count() > 1) {
                output.error(KEY_TOO_MANY_TARGET);
            } else {
                processCommand(origin, output, *result.begin(), params.range);
            }
        }
    );
    // ticking at x y z [dimension] [range]
    command.overload<TickingPosParam>()
        .text("at")
        .required("pos")
        .optional("dimension")
        .optional("range")
        .execute([](class CommandOrigin const& origin,
                    class CommandOutput&       output,
                    TickingPosParam const&     params) {
            auto pos = params.pos.getBlockPos(0, origin, origin.getWorldPosition());
            if (params.dimension >= 0) {
                auto dimRef = ll::service::getLevel()->getDimension(params.dimension);
                if (!(uintptr_t&)dimRef) {
                    output.error("Dimension not loaded");
                    return;
                }
                auto  dim    = dimRef.lock();
                auto& region = dim->getBlockSourceFromMainChunkSource();
                processCommand(origin, output, region, pos, params.range);
            } else {
                auto dim = origin.getDimension();
                if (!dim) {
                    output.error("Dimension not loaded");
                    return;
                }
                auto& region = dim->getBlockSourceFromMainChunkSource();
                processCommand(origin, output, region, pos, params.range);
            }
        });
    // ticking [range]
    command.overload<TickingRangeParam>().optional("range").execute(
        [](class CommandOrigin const& origin,
           class CommandOutput&       output,
           TickingRangeParam const&   params) {
            auto actor = origin.getEntity();
            if (actor) processCommand(origin, output, actor, params.range);
            else {
                auto& region = origin.getDimension()->getBlockSourceFromMainChunkSource();
                auto  bpos   = origin.getBlockPosition();
                processCommand(origin, output, region, bpos, params.range);
            };
        }
    );
}

} // namespace lfp