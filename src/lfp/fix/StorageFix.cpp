#include "lfp/fix/FixManager.h"

#include "mc/certificates/Certificate.h"
#include "mc/certificates/identity/GameServerToken.h"
#include "mc/common/SubClientId.h"
#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components/ActorUniqueIDComponent.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/network/NetworkIdentifier.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/level/storage/DBStorage.h"


#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"

#include "lfp/LeviFakePlayer.h"
#include "lfp/manager/FakePlayerManager.h"
#include "lfp/utils/DebugUtils.h"
#include "lfp/utils/SimulatedPlayerUtils.h"
#include <cassert>
#include <cstddef>


namespace lfp::fix {

// Save fake player data

LL_TYPE_INSTANCE_HOOK(
    SavePlayerDataFix,
    ::ll::memory::HookPriority::Normal,
    LevelStorage,
    &LevelStorage::save,
    void,
    ::Player& player
) {
    origin(player);
    if (!utils::sp_utils::isFakePlayer(player)) return;
    auto& manager = lfp::FakePlayerManager::getManager();
    auto  fp      = manager.tryGetFakePlayer(player);
    if (!fp) return;
    DEBUGW("&LevelStorage::save(::Player& player);");
    try {
        manager.saveRuntimeData(*fp);
    } catch (...) {
        manager.mLogger.error("Error occurred when saving data of \"{}\".", fp->getRealName());
    }
}

// load fake player data
LL_TYPE_INSTANCE_HOOK(
    LoadPlayerDataFix,
    ll::memory::HookPriority::Normal,
    LevelStorage,
    &LevelStorage::loadServerPlayerData,
    ::std::unique_ptr<::CompoundTag>,
    ::Player const& client,
    bool            isXboxLive
) {
    auto loadedTag = origin(
        std::forward<decltype(client)>(client),
        std::forward<decltype(isXboxLive)>(isXboxLive)
    );
    if (isXboxLive || !client.isSimulated()) return std::forward<decltype(loadedTag)>(loadedTag);

    DEBUGW("LevelStorage::loadServerPlayerData({}, {})", client.getNameTag(), isXboxLive);
    auto fp = client.getEntityContext().tryGetComponent<ActorUniqueIDComponent>().transform(
        [](auto&& comp) -> FakePlayer* {
            if (comp.mActorUniqueID == ActorUniqueID::INVALID_ID()) return nullptr;
            return lfp::FakePlayerManager::getManager().tryGetFakePlayer(comp.mActorUniqueID);
        }
    );
    if (fp) {
        if (loadedTag) {
            lfp::FakePlayerManager::getManager().mLogger.warn(
                "Storage data for SimulatedPlayer is not empty"
            );
        }
        auto& playerTag = fp->mCachedTag;
        if (playerTag) {
            DEBUGW("Replace SimulatedPlayer data");
            loadedTag.swap(playerTag);
        }
    }
    return std::forward<decltype(loadedTag)>(loadedTag);
}

// fix player identifiers before constructor
LL_TYPE_INSTANCE_HOOK(
    PlayerIdsFix,
    ::ll::memory::HookPriority::Normal,
    ::SimulatedPlayer,
    &::SimulatedPlayer::$ctor,
    void*,
    ::Level&                                           level,
    ::PacketSender&                                    packetSender,
    ::ServerNetworkSystem&                             network,
    ::ClientBlobCache::Server::ActiveTransfersManager& clientCacheMirror,
    ::GameType                                         playerGameType,
    ::NetworkIdentifier const&                         owner,
    ::SubClientId                                      subId,
    ::std::function<void(::ServerPlayer&)>             playerLoadedCallback,
    ::mce::UUID                                        uuid,
    ::std::string const&                               deviceId,
    ::GameServerToken const&                           token,
    int                                                maxChunkRadius,
    bool                                               enableItemStackNetManager,
    ::EntityContext&                                   entityContext
) {
    auto& manager = lfp::FakePlayerManager::getManager();
    if (lfp::FakePlayer::sLoggingInPlayer) {
        auto& fp                                = *lfp::FakePlayer::sLoggingInPlayer;
        uuid                                    = fp.getUuid();
        subId                                   = fp.getClientSubId();
        const_cast<::NetworkIdentifier&>(owner) = fp.FAKE_NETWORK_ID;
    } else {
        manager.mLogger.info("Unknown SimulatedPlayer creation detected");
    }

    auto ret = origin(
        std::forward<::Level&>(level),
        std::forward<::PacketSender&>(packetSender),
        std::forward<::ServerNetworkSystem&>(network),
        std::forward<::ClientBlobCache::Server::ActiveTransfersManager&>(clientCacheMirror),
        std::forward<::GameType>(playerGameType),
        std::forward<::NetworkIdentifier const&>(owner),
        std::forward<::SubClientId>(subId),
        std::forward<::std::function<void(::ServerPlayer&)>>(playerLoadedCallback),
        std::forward<::mce::UUID>(uuid),
        std::forward<::std::string const&>(deviceId),
        std::forward<::GameServerToken const&>(token),
        std::forward<int>(maxChunkRadius),
        std::forward<bool>(enableItemStackNetManager),
        std::forward<::EntityContext&>(entityContext)
    );
#ifdef LFP_DEBUG
    if (lfp::FakePlayer::sLoggingInPlayer) {
        assert(getClientSubId() == lfp::FakePlayer::sLoggingInPlayer->getClientSubId());
        assert(getUuid() == lfp::FakePlayer::sLoggingInPlayer->getUuid());
    }
    lfp::LeviFakePlayer::getLogger().debug(
        "SimulatedPlayer::$ctor: {}, client sub id: {}, runtime id: {}",
        this->getNameTag(),
        (int)this->getClientSubId(),
        this->getRuntimeID().rawID
    );
#endif
    return ret;
};

// NOLINTNEXTLINE
void FixManager::activateStorageFix() {
    LoadPlayerDataFix::hook();
    SavePlayerDataFix::hook();
    PlayerIdsFix::hook();
}

void FixManager::deactivateStorageFix() {
    LoadPlayerDataFix::unhook();
    SavePlayerDataFix::unhook();
    PlayerIdsFix::unhook();
}


} // namespace lfp::fix