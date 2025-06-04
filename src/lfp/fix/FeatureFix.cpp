#include "FixManager.h"
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/NetworkBlockPosition.h>

#include "mc/deps/core/math/Vec3.h"
#include "mc/network/LoopbackPacketSender.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/packet/PlayStatusPacket.h"
#include "mc/network/packet/PlayerActionPacket.h"
#include "mc/network/packet/RequestChunkRadiusPacket.h"
#include "mc/network/packet/RespawnPacket.h"
#include "mc/network/packet/ShowCreditsPacket.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/Minecraft.h"
#include "mc/world/actor/ActorInitializationMethod.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/chunk/ChunkViewSource.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/storage/DBStorage.h"

#include "ll/api/base/StdInt.h"
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ServerThreadExecutor.h"


#include "lfp/LeviFakePlayer.h"
#include "lfp/manager/FakePlayer.h"
#include "lfp/manager/FakePlayerManager.h"
#include "lfp/utils/DebugUtils.h"


namespace lfp::fix {

#ifdef LFP_DEBUG
template <typename T>
    requires std::is_base_of_v<Packet, T>
[[maybe_unused]] std::string packetToDebugString(T& pkt) {
    return fmt::format("pkt {}<{}>", pkt.getName(), (int)pkt.getId());
}
#endif

template <typename T>
void SimulateSendPacketToServer(SimulatedPlayer& sp, T& packet) {
    packet.mSenderSubId = sp.getClientSubId();
    DEBUGW("({})::send - {}", sp.getNameTag(), packetToDebugString(packet));
    ll::service::getServerNetworkHandler()->handle(sp.getNetworkIdentifier(), packet);
}
template <typename T>
void SimulateSendPacketToServerAfter(SimulatedPlayer& sp, T&& packet, size_t delayTicks = 1) {
    packet.mSenderSubId = sp.getClientSubId();
    DEBUGW("({})::send - {}", sp.getNameTag(), packetToDebugString(packet));
    ll::thread::ServerThreadExecutor::getDefault().executeAfter(
        [&nid = sp.getNetworkIdentifier(), packet = std::move(packet)]() {
            ll::service::getServerNetworkHandler()->handle(nid, packet);
        },
        ll::chrono::ticks{delayTicks}
    );
}


// ================= Fix chunk load =================
// fix chunk load and tick - ChunkSource load mode
LL_TYPE_INSTANCE_HOOK(
    LoadChunkFix_ChunkSource_LoadMode,
    ll::memory::HookPriority::Normal,
    SimulatedPlayer,
    &SimulatedPlayer::$_createChunkSource,
    ::std::shared_ptr<::ChunkViewSource>,
    ::ChunkSource& mainChunkSource
) {
    if (FixManager::shouldFix(this)) {
        // auto cs = origin( mainChunkSource);
        // // ChunkSource::LoadMode : None(0) -> Deferred(1)
        // cs->mParentLoadMode = ::ChunkSource::LoadMode::Deferred;
        // return cs;
        return this->ServerPlayer::$_createChunkSource(mainChunkSource);
    }
    return origin(mainChunkSource);
}

// ================= Fix tick =================
// fix chunk load and tick - _updateChunkPublisherView
LL_TYPE_INSTANCE_HOOK(
    LoadChunkFix__updateChunkPublisherView,
    ll::memory::HookPriority::Normal,
    SimulatedPlayer,
    &SimulatedPlayer::$tickWorld,
    void,
    ::Tick const& tick
) {
    origin(tick);
    //  _updateChunkPublisherView will be called after Player::tick in ServerPlayer::tick
    if (FixManager::shouldFix(this)) {
        // Force to call the implementation of ServerPlayer
        // seem this only for tell client which chunk ready in server
        this->ServerPlayer::$_updateChunkPublisherView(getPosition(), 16.0f);
    }
}

// ================== Fix Travel ==================
// First change dimension from the end
LL_TYPE_INSTANCE_HOOK(
    TravelFix_ShowCredits,
    ll::memory::HookPriority::Normal,
    SimulatedPlayer,
    &SimulatedPlayer::$changeDimensionWithCredits,
    void,
    ::DimensionType dimension
) {
    origin(dimension);
    if (FixManager::shouldFix(this)) {
        // mHasSeenCredits = true;
        ShowCreditsPacket packet{};
        packet.mPlayerID     = getRuntimeID();
        packet.mCreditsState = ShowCreditsPacket::CreditsState::Finished;
        SimulateSendPacketToServerAfter(*this, packet);
    }
}

[[maybe_unused]] void SimulateHandlePacketFromServer(
    SimulatedPlayer&                          sp,
    [[maybe_unused]] ShowCreditsPacket const& packet
) {
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToDebugString(packet));
    ShowCreditsPacket res;
    res.mPlayerID     = sp.getRuntimeID();
    res.mCreditsState = ShowCreditsPacket::CreditsState::Finished;
    SimulateSendPacketToServerAfter(sp, res, 1);
    return;
}
// fix state when changing dimension


// ================== Fix Respawn ==================

LL_TYPE_INSTANCE_HOOK(
    RespawnFix_onPlayerDie,
    ll::memory::HookPriority::Normal,
    SimulatedPlayer,
    &SimulatedPlayer::$die,
    void,
    ::ActorDamageSource const& source
) {
    if (FixManager::shouldFix(this)) {
        RespawnPacket packet{};
        packet.mSenderSubId = getClientSubId();
        packet.mPos         = Vec3::ZERO();
        packet.mState       = PlayerRespawnState::ClientReadyToSpawn;
        packet.mRuntimeId   = getRuntimeID();
        SimulateSendPacketToServerAfter(*this, std::move(packet));
    }
    return origin(std::forward<::ActorDamageSource const&>(source));
};


void SimulateHandlePacketFromServer(SimulatedPlayer& sp, RespawnPacket const& packet) {
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToDebugString(packet));
    auto state = packet.mState;

    if (state == PlayerRespawnState::SearchingForSpawn) {
        // sp.setBlockRespawnUntilClientMessage(false);
        RespawnPacket res{};
        res.mSenderSubId = sp.getClientSubId();
        res.mPos         = Vec3::ZERO();
        res.mState       = PlayerRespawnState::ClientReadyToSpawn;
        res.mRuntimeId   = sp.getRuntimeID();
        SimulateSendPacketToServerAfter(sp, std::move(res));
        // assert(sp.mBlockRespawnUntilClientMessage);
    } else {
        PlayerActionPacket res{};
        res.mPos = Vec3::ZERO();
        // res.mResultPos                        = ;
        res.mFace      = -1;
        res.mAction    = PlayerActionType::Respawn;
        res.mRuntimeId = sp.getRuntimeID();
        // res.mIsFromServerPlayerMovementSystem = ;

        SimulateSendPacketToServerAfter(sp, std::move(res));
    }
}

// ================== Fix Initialized Spawn ==================
void SimulateHandlePacketFromServer(
    SimulatedPlayer&                         sp,
    [[maybe_unused]] PlayStatusPacket const& packet
) {
    // SpawnFix chunk radius
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToDebugString(packet));
    static int ChunkRadius = lfp::LeviFakePlayer::getInstance().getConfig().config.fix.chunkRadius;

    RequestChunkRadiusPacket res;
    res.mChunkRadius    = ChunkRadius;
    res.mMaxChunkRadius = static_cast<uchar>(ChunkRadius);

    SimulateSendPacketToServerAfter(sp, res, 5);
}

LL_TYPE_INSTANCE_HOOK(
    SpawnFix_respawnPos,
    ll::memory::HookPriority::Normal,
    Player,
    &Player::setSpawnBlockRespawnPosition,
    bool,
    ::BlockPos const& spawnBlockPosition,
    ::DimensionType   dimension
) {
    if (auto fp = FixManager::tryGetFakePlayer(this)) {

        if (!fp->isOnline() && !fp->mCreateAt) return false;
    }
    return origin(
        std::forward<::BlockPos const&>(spawnBlockPosition),
        std::forward<::DimensionType>(dimension)
    );
};

/// TODO: trigger builtin spawn point search logic
LL_TYPE_INSTANCE_HOOK(
    SpawnFix_Temp_initSpawnPos,
    ll::memory::HookPriority::Normal,
    SimulatedPlayer,
    &SimulatedPlayer::$initializeComponents,
    void,
    ::ActorInitializationMethod   method,
    ::VariantParameterList const& params
) {
    if (method == ActorInitializationMethod::Updated) {
        if (FixManager::shouldFix(*this, true)) {
            BlockPos bpos = getPosition();
            if (bpos.y > 10000 && _getRegion().findNextTopSolidBlockUnder(bpos)) {
                teleportTo(bpos, false, 0, 0, false);
            }
        }
    }
    return origin(
        std::forward<::ActorInitializationMethod>(method),
        std::forward<::VariantParameterList const&>(params)
    );
};

bool SimulateHandlePacketFromServer(
    NetworkIdentifier const& nid,
    Packet const&            packet,
    SubClientId              subId
) {
    auto sp = ll::service::getServerNetworkHandler()->_getServerPlayer(nid, subId);
    if (!FixManager::shouldFix(sp)) return false;
    const auto packetId = packet.getId();
    switch (packetId) {
    case MinecraftPacketIds::Respawn:
        SimulateHandlePacketFromServer(
            *static_cast<SimulatedPlayer*>(sp),
            static_cast<RespawnPacket const&>(packet)
        );
        break;
    case MinecraftPacketIds::ShowCredits:
        SimulateHandlePacketFromServer(
            *static_cast<SimulatedPlayer*>(sp),
            static_cast<ShowCreditsPacket const&>(packet)
        );
        break;
    case MinecraftPacketIds::PlayStatus:
        SimulateHandlePacketFromServer(
            *static_cast<SimulatedPlayer*>(sp),
            static_cast<PlayStatusPacket const&>(packet)
        );
        break;
    default:
        break;
    }
    return true;
}

// ================== Hook server outgoing packet ==================
LL_TYPE_INSTANCE_HOOK(
    FixByPacketHook,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClient,
    void,
    ::UserEntityIdentifierComponent const* userIdentifier,
    ::Packet const&                        packet
) {
    /// TODO:
    if (userIdentifier->mNetworkId == lfp::FakePlayer::FAKE_NETWORK_ID) {
        try {
            [[maybe_unused]] auto handled = SimulateHandlePacketFromServer(
                userIdentifier->mNetworkId,
                packet,
                userIdentifier->mClientSubId
            );
            // if (handled) return;
        } catch (...) {
            lfp::LeviFakePlayer::getLogger().error(
                "Error in handle packet for SubClient<{}>",
                (int)userIdentifier->mClientSubId
            );
        }
    }
    origin(
        std::forward<::UserEntityIdentifierComponent const*>(userIdentifier),
        std::forward<::Packet const&>(packet)
    );
};

void FixManager::activateFeatureFix() {
    // LoadChunkFix__updateChunkPublisherView::hook();
    LoadChunkFix_ChunkSource_LoadMode::hook();
    // RespawnFix_onPlayerDie::hook();
    TravelFix_ShowCredits::hook();
    SpawnFix_Temp_initSpawnPos::hook();
    SpawnFix_respawnPos::hook();
    FixByPacketHook::hook();
}

void FixManager::deactivateFeatureFix() {
    // LoadChunkFix__updateChunkPublisherView::unhook();
    LoadChunkFix_ChunkSource_LoadMode::unhook();
    // RespawnFix_onPlayerDie::unhook();
    TravelFix_ShowCredits::unhook();
    SpawnFix_Temp_initSpawnPos::hook();
    SpawnFix_respawnPos::hook();
    FixByPacketHook::unhook();
}

}; // namespace lfp::fix