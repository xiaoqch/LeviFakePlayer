#pragma once

#include "mc/deps/core/utility/optional_ref.h"
#include "mc/gametest/MinecraftGameTest.h"
#include "mc/gametest/MinecraftGameTestHelperProvider.h"
#include "mc/gametest/framework/BaseGameTestHelper.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/platform/UUID.h"
#include "mc/scripting/modules/gametest/ScriptSimulatedPlayer.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/Minecraft.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/storage/PlayerDataSystem.h"
#include "mc/world/level/storage/PlayerSaveSuspensionComponent.h"
#include "mc/world/level/storage/PlayerSuspendLevelStorageSaveToken.h"

#include "ll/api/service/Bedrock.h"

namespace lfp::utils::sp_utils {

constexpr Vec2 DEFAULT_ROTATION = Vec2{0, 0};


std::string xuidFromUuid(mce::UUID uuid);

mce::UUID uuidFromName(std::string_view name);

optional_ref<gametest::BaseGameTestHelper> getGameTestHelper();

inline bool isFakePlayer(Player const* player) { return player && player->isSimulated(); }
inline bool isFakePlayer(Player const& player) { return isFakePlayer(&player); }

inline optional_ref<SimulatedPlayer> create(
    std::string const&           name,
    BlockPos const&              spawnPos,
    DimensionType                dimensionId,
    std::string                  xuid          = {},
    std::optional<ActorUniqueID> idOverride    = {},
    std::optional<Vec3>          spawnPosDelta = {},
    std::optional<Vec2>          spawnRotation = {}
) {
    auto serverNetworkHandler = ll::service::getServerNetworkHandler();
    if (!serverNetworkHandler) {
        return nullptr;
    }
    if (xuid.empty()) xuid = xuidFromUuid(uuidFromName(name));
    bool spawnLoadedFromSave = false;

    auto player = SimulatedPlayer::create(
        name,
        spawnPos,
        spawnPosDelta.value_or(Vec3::ZERO()),
        spawnRotation.value_or(DEFAULT_ROTATION),
        spawnLoadedFromSave,
        dimensionId,
        *serverNetworkHandler,
        xuid,
        idOverride
    );

    return player;
}

inline SimulatedPlayer* tryGetSimulatedPlayer(Actor* actor) {
    try {
        if (actor && actor->isSimulatedPlayer()) return (SimulatedPlayer*)actor;
        return nullptr;
    } catch (...) {}
    return nullptr;
}

// for import data from [FakePlayer](https://github.com/ddf8196/FakePlayer)
mce::UUID JAVA_nameUUIDFromBytes(std::string_view name);


}; // namespace lfp::utils::sp_utils
