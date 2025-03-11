#include "lfp/fix/FixManager.h"

#include "mc/certificates/identity/GameServerToken.h"
#include "mc/common/SubClientId.h"
#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components/ActorUniqueIDComponent.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/level/storage/DBStorage.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"

#include "lfp/LeviFakePlayer.h"
#include "lfp/manager/FakePlayerManager.h"
#include "lfp/utils/DebugUtils.h"
#include "lfp/utils/SimulatedPlayerUtils.h"


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
        lfp::FakePlayerManager::getManager().saveRuntimeData(*fp);
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
    DEBUGW("LevelStorage::loadServerPlayerData({}, {})", client.getNameTag(), isXboxLive);
    if (FixManager::shouldFix(client, true)) {
        auto& manager = lfp::FakePlayerManager::getManager();
        if (loadedTag) {
            manager.mLogger.warn("Storage data for SimulatedPlayer is not empty");
        }
        auto fp = manager.tryGetFakePlayerByXuid(client.getXuid());
        if (fp) {
            auto& playerTag = fp->mCachedTag;
            if (playerTag) {
                DEBUGW("Replace SimulatedPlayer data");
                loadedTag.swap(playerTag);
            }
        }
    }
    return std::forward<decltype(loadedTag)>(loadedTag);
}

#if false

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
    ::SubClientId                                      subid,
    ::std::function<void(::ServerPlayer&)>             playerLoadedCallback,
    ::mce::UUID                                        uuid,
    ::std::string const&                               token,
    ::GameServerToken const&                           maxChunkRadius,
    int                                                enableItemStackNetManager,
    bool                                               entityContext,
    ::EntityContext&                                   deviceId
) {

    auto& manager = lfp::FakePlayerManager::getManager();
    if (lfp::FakePlayer::mLoggingInPlayer) {
        auto& fp = *lfp::FakePlayer::mLoggingInPlayer;
        uuid     = fp.getUuid();
        subid    = fp.getClientSubId();
        if (fp.mUniqueId != ActorUniqueID::INVALID_ID()) {
            auto uidcmp = deviceId.getOrAddComponent<ActorUniqueIDComponent>(fp.mUniqueId);
            assert(uidcmp.mActorUniqueID == fp.mUniqueId);
        }
        const_cast<::NetworkIdentifier&>(owner) = fp.FAKE_NETWORK_ID;
    } else {
        manager.mLogger.warn("Unknown SimulatedPlayer creation detected");
    }

    auto rtn = origin(
        std::forward<::Level&>(level),
        std::forward<::PacketSender&>(packetSender),
        std::forward<::ServerNetworkSystem&>(network),
        std::forward<::ClientBlobCache::Server::ActiveTransfersManager&>(clientCacheMirror),
        std::forward<::GameType>(playerGameType),
        std::forward<::NetworkIdentifier const&>(owner),
        std::forward<::SubClientId>(subid),
        std::forward<::std::function<void(::ServerPlayer&)>>(playerLoadedCallback),
        std::forward<::mce::UUID>(uuid),
        std::forward<::std::string const&>(token),
        std::forward<::GameServerToken const&>(maxChunkRadius),
        std::forward<int>(enableItemStackNetManager),
        std::forward<bool>(entityContext),
        std::forward<::EntityContext&>(deviceId)
    );
    assert(getClientSubId() == subid);
    lfp::LeviFakePlayer::getLogger().debug(
        "SimulatedPlayer::$ctor: {}, client sub id: {}, runtime id: {}",
        this->getNameTag(),
        (int)this->getClientSubId(),
        this->getRuntimeID().rawID
    );
    return rtn;
};

#else

// fix player identifiers before constructor
LL_TYPE_INSTANCE_HOOK(
    PlayerIdsFix,
    ::ll::memory::HookPriority::Normal,
    ::SimulatedPlayer,
    &::Player::$ctor,
    void*,
    ::Level&                   level,
    ::PacketSender&            packetSender,
    ::GameType                 playerGameType,
    bool                       isHostingPlayer,
    ::NetworkIdentifier const& owner,
    ::SubClientId              subid,
    ::mce::UUID                uuid,
    ::std::string const&       playFabId,
    ::std::string const&       deviceId,
    ::GameServerToken const&   gameServerToken,
    ::EntityContext&           entityContext,
    ::std::string const&       platformId,
    ::std::string const&       platformOnlineId
) {
    bool shouldFix =
        playFabId.empty() && deviceId.empty() && owner.mType == NetworkIdentifier::Type::Invalid;
    if (shouldFix) {
        auto& manager = lfp::FakePlayerManager::getManager();
        if (lfp::FakePlayer::mLoggingInPlayer) {
            auto& fp = *lfp::FakePlayer::mLoggingInPlayer;
            uuid     = fp.getUuid();
            subid    = fp.getClientSubId();
            if (fp.mUniqueId != ActorUniqueID::INVALID_ID()) {
                auto uidcmp = entityContext.getOrAddComponent<ActorUniqueIDComponent>(fp.mUniqueId);
            }
            const_cast<::NetworkIdentifier&>(owner) = fp.FAKE_NETWORK_ID;
        } else {
            manager.mLogger.warn("Unknown SimulatedPlayer creation detected");
        }
    }
    auto rtn = origin(
        std::forward<::Level&>(level),
        std::forward<::PacketSender&>(packetSender),
        std::forward<::GameType>(playerGameType),
        std::forward<bool>(isHostingPlayer),
        std::forward<::NetworkIdentifier const&>(owner),
        std::forward<::SubClientId>(subid),
        std::forward<::mce::UUID>(uuid),
        std::forward<::std::string const&>(playFabId),
        std::forward<::std::string const&>(deviceId),
        std::forward<::GameServerToken const&>(gameServerToken),
        std::forward<::EntityContext&>(entityContext),
        std::forward<::std::string const&>(platformId),
        std::forward<::std::string const&>(platformOnlineId)
    );
#ifdef LFP_DEBUG
    if (shouldFix) {
        assert(getClientSubId() == subid);
        lfp::LeviFakePlayer::getLogger().debug(
            "SimulatedPlayer::$ctor: {}, client sub id: {}, runtime id: {}",
            getNameTag(),
            (int)getClientSubId(),
            getRuntimeID().rawID
        );
    }
#endif
    return rtn;
};
#endif

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