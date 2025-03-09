#pragma once

#include "mc/network/ServerNetworkHandler.h"
#include "mc/platform/UUID.h"
#include "mc/scripting/modules/gametest/ScriptSimulatedPlayer.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/level/BlockPos.h"

#include "ll/api/service/Bedrock.h"

namespace lfp::utils::sp_utils {


std::string xuidFromUuid(mce::UUID uuid);

mce::UUID uuidFromName(std::string_view name);


inline bool isFakePlayer(Player const* player) {
    return player && player->isSimulated()
        && nullptr
               == ScriptModuleGameTest::ScriptSimulatedPlayer::_getHelper(*(SimulatedPlayer*)player
               );
}
inline bool isFakePlayer(Player const& player) { return isFakePlayer(&player); }


// Rewrite SimulatedPlayer::create(std::string const & name,class BlockPos const & bpos,class
// AutomaticID<class Dimension,int> dimId,class ServerNetworkHandler & serverNetworkHandler)
inline SimulatedPlayer* create(
    std::string const& name,
    BlockPos const&    spawnPos,
    DimensionType      dimensionId,
    std::string        xuid = {}
) {
    auto serverNetworkHandler = ll::service::getServerNetworkHandler();
    if (!serverNetworkHandler) {
        return nullptr;
    }
    if (xuid.empty()) xuid = xuidFromUuid(uuidFromName(name));
    auto player = SimulatedPlayer::create(name, spawnPos, dimensionId, *serverNetworkHandler, xuid);
    if (!player) {
        return nullptr;
    }
    // auto             ownerPtr = serverNetworkHandler->createSimulatedPlayer(name, "");
    // SimulatedPlayer* player   = ownerPtr.tryGetSimulatedPlayer();
    // if (player /* && player->isSimulatedPlayer() */) {
    //     // dAccess<AutomaticID<Dimension, int>>(player, 57) = dimId;
    //     Level& level = player->getLevel();
    //     player->setDimension(level.getDimension(dimId));
    //     // player->postLoad(/* isNewPlayer */ false);
    //     ll::service::getLevel()->addUser(std::move(ownerPtr));
    //     auto pos = bpos.bottomCenter();
    //     pos.y = pos.y + 1.62001;
    //     player->setPos(pos);
    //     // player->setRespawnReady(pos);
    //     player->mRespawnReady = true;
    //     player->setSpawnBlockRespawnPosition(bpos, dimId);
    //     player->setLocalPlayerAsInitialized();
    // }
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
