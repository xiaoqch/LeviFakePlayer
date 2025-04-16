
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#ifdef LFP_DEBUG

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>


#include "fmt/core.h"
#include "magic_enum.hpp"
#include "mc/certificates/Certificate.h"
#include "mc/certificates/CertificateSNIType.h"
#include "mc/certificates/UnverifiedCertificate.h"
#include "mc/certificates/identity/GameServerToken.h"
#include "mc/common/SubClientId.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components/ServerAuthMovementMode.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/NbtIo.h"
#include "mc/nbt/Tag.h"
#include "mc/network/LoopbackPacketSender.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/NetEventCallback.h"
#include "mc/network/NetworkBlockPosition.h"
#include "mc/network/NetworkIdentifier.h"
#include "mc/network/NetworkIdentifierWithSubId.h"
#include "mc/network/PacketSender.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/ServerNetworkSystem.h"
#include "mc/network/packet/ModalFormCancelReason.h"
#include "mc/network/packet/NetworkChunkPublisherUpdatePacket.h"
#include "mc/network/packet/RequestChunkRadiusPacket.h"
#include "mc/network/packet/SetLocalPlayerAsInitializedPacket.h"
#include "mc/platform/UUID.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/ServerPlayer.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/server/blob_cache/ActiveTransfersManager.h"
#include "mc/world/actor/player/PlayerMovementSettings.h"
#include "mc/world/containers/managers/models/ContainerManagerModel.h"
#include "mc/world/events/ActorCarriedItemChangedEvent.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/NetworkItemStackDescriptor.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/ILevel.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/chunk/ChunkSource.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/storage/DBStorage.h"
#include "mc/world/level/storage/LevelStorage.h"

#include "lfp/utils/packets.h"
#include "mc/network/packet/ChangeDimensionPacket.h"
#include "mc/network/packet/InventorySlotPacket.h"
#include "mc/network/packet/MobEquipmentPacket.h"
#include "mc/network/packet/ModalFormRequestPacket.h"
#include "mc/network/packet/ModalFormResponsePacket.h"
#include "mc/network/packet/MovePlayerPacket.h"
#include "mc/network/packet/Packet.h"
#include "mc/network/packet/PlayStatusPacket.h"
#include "mc/network/packet/PlayerActionPacket.h"
#include "mc/network/packet/PlayerHotbarPacket.h"
#include "mc/network/packet/RespawnPacket.h"
#include "mc/network/packet/ShowCreditsPacket.h"

#include "ll/api/io/Logger.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/reflection/TypeName.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ServerThreadExecutor.h"

#include "lfp/LeviFakePlayer.h"
#include "lfp/command/TickingCommand.h"
#include "lfp/manager/FakePlayer.h"
#include "lfp/utils/ColorUtils.h"
#include "lfp/utils/DebugUtils.h"
#include "lfp/utils/SimulatedPlayerUtils.h"


namespace {

auto& logger = lfp::LeviFakePlayer::getLogger();
template <typename T>
    requires std::is_base_of_v<Packet, T>
std::string packetToString(T& pkt) {
    return fmt::format(
        "pkt {}<{}>",
        // ", P: {}, R: {}, C: {}, Z: {}",
        magic_enum::enum_name(pkt.getId()),
        (int)pkt.getId()
        // magic_enum::enum_name(pkt.mPriority),
        // magic_enum::enum_name(pkt.mReliability),
        // magic_enum::enum_name(pkt.mClientSubId),
        // magic_enum::enum_name(pkt.mCompressible)
    );
}

} // namespace

namespace Schedule {

void delay(std::function<void()>&& func, size_t ticks) {
    ll::thread::ServerThreadExecutor::getDefault().executeAfter(func, ll::chrono::ticks{ticks});
}
void nextTick(std::function<void()>&& func) { return delay(std::move(func), 0); }

} // namespace Schedule


namespace SimulatedClient {

template <typename T>
void send(SimulatedPlayer const& sp, T& packet) {
    packet.mClientSubId = sp.getClientSubId();
    DEBUGW("({})::send - {}", sp.getNameTag(), packetToString(packet));
    ll::service::getServerNetworkHandler()->handle(sp.getNetworkIdentifier(), packet);
}


// ================== Fix Change Dimension ==================

// HOOK_LoopbackPacketSender_$sendToClient2 called ChangeDimensionPacket - 61
void handle(SimulatedPlayer& sp, ChangeDimensionPacket const& packet) {
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToString(packet));
    auto uniqueId = sp.getOrCreateUniqueID();
    auto pos      = packet.mPos;

    // (PlayerActionType::DimensionChangeDone, Vec3::ZERO, 0, sp.getRuntimeID());
    PlayerActionPacket res;
    res.mPos = Vec3::ZERO();
    // res.mResultPos = ;
    // res.mFace      = -1;
    res.mAction    = PlayerActionType::ChangeDimensionAck;
    res.mRuntimeId = sp.getRuntimeID();
    // res.mIsFromServerPlayerMovementSystem = ;
    send(sp, res);
}


// HOOK_ServerPlayer_$sendNetworkPacket1 called
// - HOOK_LoopbackPacketSender_$sendToClient2 called ShowCreditsPacket - 75
// void handle(SimulatedPlayer& sp, ShowCreditsPacket const& packet) {
//     DEBUGW("({})::handle - {}", sp.getNameTag(), packetToString(packet));
//     auto uniqueId = sp.getOrCreateUniqueID();
//     Schedule::delay(
//         [&sp]() {
//             // TODO: uniqueId 失效
//             // auto sp = (SimulatedPlayer*)ll::service::getLevel()->fetchEntity(uniqueId, true);
//             ASSERT(lfp::utils::sp_utils::isFakePlayer(sp));
//             ShowCreditsPacket res;
//             res.mPlayerID     = sp.getRuntimeID();
//             res.mCreditsState = ShowCreditsPacket::CreditsState::Finished;
//             send(sp, res);
//         },
//         10
//     );
// }


void handle(SimulatedPlayer& sp, ModalFormRequestPacket const& packet) {
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToString(packet));
    auto uniqueId = sp.getOrCreateUniqueID();
    auto formId   = packet.mFormId;
    Schedule::delay(
        [uniqueId, formId]() {
            auto pl = ll::service::getLevel()->getPlayer(uniqueId);
            ASSERT(pl->isSimulated());
            auto& sp = *(SimulatedPlayer*)pl;
            // auto res = MinecraftPackets::createPacket(MinecraftPacketIds::ModalFormResponse);
            ModalFormResponsePacket res;
            res.mFormId           = formId;
            res.mJSONResponse     = "null";
            res.mFormCancelReason = ::ModalFormCancelReason::UserClosed;
            res.mIsHandled        = false;
            res.mClientSubId      = sp.getClientSubId();
            send(sp, res);
        },
        20
    );
}

void handle(SimulatedPlayer& sp, MovePlayerPacket const& packet) {
    // if (sp.getRuntimeID() == packet.mPlayerID) {
    //     dAccess<float>(sp, 468) = 0;
    //     // sp.setPosPrev(sp.getPos());
    // }
    // DEBUGW("({})::handle - {}", sp.getNameTag(), packetToString(packet));
    // auto uniqueId = sp.getOrCreateUniqueID();
    // Schedule::delay([uniqueId]() { auto sp = ll::service::getLevel()->getPlayer(uniqueId); },
    // 20);
}

// Fix weapon and armor display
void handle(SimulatedPlayer& sp, [[maybe_unused]] InventorySlotPacket const& packet) {
#ifdef LFP_DEBUG
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToString(packet));
    auto ctn = sp.mContainerManager;
    if (ctn) sp.refreshContainer(*ctn);
#endif // LFP_DEBUG
}

// Fix weapon and armor display
void handle(SimulatedPlayer& sp, [[maybe_unused]] PlayerHotbarPacket const& packet) {
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToString(packet));
    auto ctn = sp.mContainerManager;
    if (ctn) sp.refreshContainer(*ctn);
}


void handle(SimulatedPlayer& sp, [[maybe_unused]] MobEquipmentPacket const& packet) {
#ifdef LFP_DEBUG
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToString(packet));
    // if (packet.mRuntimeId == sp.getRuntimeID()) {
    //     sp.sendInventory(true);
    // }
#endif // LFP_DEBUG
}

void handle(SimulatedPlayer& sp, [[maybe_unused]] NetworkChunkPublisherUpdatePacket const& packet) {
#ifdef LFP_DEBUG
    if (packet.mServerBuiltChunks->size() > 49) {
        SetLocalPlayerAsInitializedPacket pkt;
        pkt.mPlayerID = sp.getRuntimeID();
        send(sp, pkt);
    }
    DEBUGW("({})::handle - {}", sp.getNameTag(), packetToString(packet));
    // if (packet.mRuntimeId == sp.getRuntimeID()) {
    //     sp.sendInventory(true);
    // }
#endif // LFP_DEBUG
}
/*
execute at @p run setblock ~ ~ ~ portal
execute as @p run ticking
*/

void handlePacket(SimulatedPlayer& sp, Packet const& packet) {
    auto packetId = packet.getId();
    switch (packetId) {
    // case MinecraftPacketIds::ShowCredits:
    //     SimulatedClient::handle(sp, (ShowCreditsPacket const&)packet);
    // break;
    case MinecraftPacketIds::ChangeDimension:
        SimulatedClient::handle(sp, (ChangeDimensionPacket const&)packet);
        break;
    // case MinecraftPacketIds::StartGame:
    //     // Not be sent to simulated player, so listen to PlayStatusPacket instead
    //     SimulatedClient::handle(sp, (StartGamePacket*)&packet);
    //     break;
    case MinecraftPacketIds::PlayerHotbar:
        SimulatedClient::handle(sp, (PlayerHotbarPacket const&)packet);
        break;
    // case MinecraftPacketIds::PlayStatus:
    //     SimulatedClient::handle(sp, (PlayStatusPacket const&)packet);
    //     break;
    // case MinecraftPacketIds::MovePlayer:
    //     SimulatedClient::handle(sp, (MovePlayerPacket const&)packet);
    //     break;
    case MinecraftPacketIds::ShowModalForm:
        SimulatedClient::handle(sp, (ModalFormRequestPacket const&)packet);
        break;
    case MinecraftPacketIds::InventorySlot:
        SimulatedClient::handle(sp, (InventorySlotPacket const&)packet);
        break;
    case MinecraftPacketIds::MobArmorEquipment:
        SimulatedClient::handle(sp, (MobEquipmentPacket const&)packet);
        break;
        // case MinecraftPacketIds::NetworkChunkPublisherUpdate:
        //     SimulatedClient::handle(sp, (NetworkChunkPublisherUpdatePacket const&)packet);
        //     break;
        // case MinecraftPacketIds::MoveAbsoluteActor:
        //     SimulatedClient::handle(sp, (MoveActorAbsolutePacket const&)packet);
        //     break;

#ifdef LFP_DEBUG
    case MinecraftPacketIds::MoveDeltaActor:
    // case MinecraftPacketIds::NetworkChunkPublisherUpdate:
    case MinecraftPacketIds::SetActorMotion:
    case MinecraftPacketIds::ActorEvent:
    case MinecraftPacketIds::UpdateAttributes:
    case MinecraftPacketIds::SetActorData:
    case MinecraftPacketIds::InventoryContent:
    case MinecraftPacketIds::LevelSoundEvent:
    case MinecraftPacketIds::RemoveActor:
        break;
        // {
        //     BinaryStream bs;
        //     packet.write(bs);
        //     logger.warn("getUnreadLength: {}", bs.mHasOverflowed);
        //     logger.warn("getReadPointer: {}", bs.mReadPointer);
        //     bs.setReadPointer(0);
        //     logger.warn(
        //         "Actor Runtime Id: {} - {}",
        //         *bs.getUnsignedVarInt64(),
        //         (size_t)sp.getRuntimeID()
        //     );
        //     int count = *bs.getUnsignedVarInt();
        //     logger.warn("count {}", count);
        //     if (count > 100) count = 100;
        //     for (int i = 0; i < count; ++i) {
        //         logger.warn(
        //             "[{}, {}, {}, {}, {}]",
        //             *bs.getString(10000),
        //             *bs.getFloat(),
        //             *bs.getFloat(),
        //             *bs.getFloat(),
        //             *bs.getFloat()
        //         );
        //     }
        //     logger.warn("tick: {}", *bs.getUnsignedVarInt64());

        //     logger.warn("getReadPointer: {}", bs.mReadPointer);
        //     break;
        // }
    // case MinecraftPacketIds::PlayerList: {
    //     int a = 1;
    // }
    default:
        logger.debug("({})::handle - {}", sp.getNameTag(), packetToString(packet));
#else
    default:
#endif // LFP_DEBUG
        break;
    }
}

bool handlePacket(NetworkIdentifier const& nid, Packet const& packet, SubClientId subId) {
    auto sp = ll::service::getServerNetworkHandler()->_getServerPlayer(nid, subId);
    if (!lfp::utils::sp_utils::isFakePlayer(sp)) return false;
    SimulatedClient::handlePacket(*(SimulatedPlayer*)sp, packet);
    return true;
}

} // namespace SimulatedClient

LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_LoopbackPacketSender_$sendToClient,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClient,
    void,
    ::UserEntityIdentifierComponent const* userIdentifier,
    ::Packet const&                        packet
) {
    if (userIdentifier->mNetworkId == lfp::FakePlayer::FAKE_NETWORK_ID) {
        try {
            if (SimulatedClient::handlePacket(
                    userIdentifier->mNetworkId,
                    packet,
                    userIdentifier->mClientSubId
                )) {
                // return;
            }
        } catch (...) {}
    }
    origin(
        std::forward<::UserEntityIdentifierComponent const*>(userIdentifier),
        std::forward<::Packet const&>(packet)
    );
};


#if false
// Perform affect
// clientSubId is necessary to identify SimulatedPlayer because they have the same networkID
LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerNetworkSystem_send,
    ll::memory::HookPriority::Normal,
    ServerNetworkSystem,
    &ServerNetworkSystem::send,
    void,
    ::NetworkIdentifier const& id,
    ::Packet const&            packet,
    ::SubClientId              senderSubId
) {
    if (id == lfp::FakePlayer::mNetworkId) {
        try {
            auto sp = ll::service::getServerNetworkHandler()->_getServerPlayer(id, senderSubId);
            if (lfp::utils::sp_utils::isFakePlayer(sp))
                return SimulatedClient::handlePacket(*(SimulatedPlayer*)sp, packet);
            if (packet.getId() == MinecraftPacketIds::InventorySlot) {
                auto& pkt = *(InventorySlotPacket*)&packet;
                Schedule::nextTick([senderSubId,
                                    ctn  = pkt.mInventoryId,
                                    slot = pkt.mSlot,
                                    item = ItemStack::fromDescriptor(
                                        pkt.mItem,
                                        ll::service::getLevel()->getBlockPalette(),
                                        false
                                    ),
                                    &id]() {
                    auto sp =
                        ll::service::getServerNetworkHandler()->_getServerPlayer(id, senderSubId);

                    if (lfp::utils::sp_utils::isFakePlayer(sp)) {
                        InventorySlotPacket packet(ctn, slot, item);
                        return SimulatedClient::handlePacket(*(SimulatedPlayer*)sp, packet);
                    } else {
                        __debugbreak();
                    }
                });
            }
            return;
        } catch (const std::exception&) {
            logger.error("Error in NetworkHandler::send");
        }
    }
    return origin(id, packet, senderSubId);
}

bool processed = false;
LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_LoopbackPacketSender_$sendToClients,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClients,
    void,
    ::std::vector<::NetworkIdentifierWithSubId> const& ids,
    ::Packet const&                                    packet
) {
    // fix simulated player sub id
    // #ifdef LFP_DEBUG
    //    logger.info("[sendToClients] -> {} clients: {}({})", clients.size(), packet.getName(),
    //    packet.getId());
    // #endif // LFP_DEBUG

    for (auto const& clientId : ids) {
        if (clientId.id.isUnassigned() && clientId.id == lfp::FakePlayer::mNetworkId) {
            try {
                auto sp = ll::service::getServerNetworkHandler()->_getServerPlayer(
                    clientId.id,
                    clientId.subClientId
                );
                if (lfp::utils::sp_utils::isFakePlayer(sp)) {
                    SimulatedClient::handlePacket(*(SimulatedPlayer*)sp, packet);
                }
            } catch (const std::exception&) {
                logger.error("Failed to get player's client sub id from NetworkIdentifier");
            }
        }
    }
    processed = true;
    origin(ids, packet);
    processed = false;
}
#endif

// LL_AUTO_TYPE_INSTANCE_HOOK(
//     LFP_NetworkHandler__sendInternal,
//     ll::memory::HookPriority::Normal,
//     ServerNetworkSystem,
//     &ServerNetworkSystem::sendint,
//     void,
//     ::NetworkIdentifier const& id,
//     ::Packet const&            packet,
//     ::SubClientId              senderSubId
// ){
//     // fix simulated player sub id
//     if (!processed && nid == FakePlayer::mNetworkID) {
//         // ASSERT(false);
//         try {
//             auto subId = dAccess<unsigned char>(&nid, 160);
//             auto sp    = Global<ServerNetworkHandler>->getServerPlayer(nid, subId);
//             if (!lfp::utils::sp_utils::isFakePlayer(sp)) {
//                 DEBUGW("try fix sub id failed - {}", pkt.getName());
//                 sp = Global<ServerNetworkHandler>->getServerPlayer(nid, pkt.clientSubId);
//                 __debugbreak();
//             }
//             if (lfp::utils::sp_utils::isFakePlayer(sp)) {
//                 return SimulatedClient::handlePacket((SimulatedPlayer*)sp, &pkt);
//             }
//         } catch (const std::exception&) {
//             logger.error("Failed to get player's client sub id from NetworkIdentifier");
//             __debugbreak();
//         }
//     }

//     origin( nid, pkt, data);
// }


// this value only in SimulatedPlayer, and used in SimulatedPlayer::aiStep for check SimulatedPlayer
// fall distance
#if false
void trySetOldY(SimulatedPlayer& sp, float y) {
#ifndef LFP_DEBUG
    static
#endif // !LFP_DEBUG
        uintptr_t offsetOldY = ([](SimulatedPlayer& sp, float oldY) -> uintptr_t {
            constexpr uintptr_t defaultOffset = 9408;
            if (oldY == dAccess<float>(&sp, defaultOffset)) return 9408;
            for (uintptr_t off = 1; off < 20; ++off) {
                if (oldY == dAccess<float>(&sp, defaultOffset + off * 4))
                    return defaultOffset + off * 4;
                if (oldY == dAccess<float>(&sp, defaultOffset - off * 4))
                    return defaultOffset - off * 4;
            }
#ifdef LFP_DEBUG
            if (FLT_MAX == dAccess<float>(&sp, defaultOffset)) return 9408;
            for (uintptr_t off = 1; off < 20; ++off) {
                if (FLT_MAX == dAccess<float>(&sp, defaultOffset + off * 4))
                    return defaultOffset + off * 4;
                if (FLT_MAX == dAccess<float>(&sp, defaultOffset - off * 4))
                    return defaultOffset - off * 4;
            }
#endif // LFP_DEBUG

            logger.error("Error in fix simulated player teleport fall damage, the offset was "
                         "changed and the automatic offset detection failed");
            __debugbreak();
            return 0;
        })(sp, sp.getPosOld().y);
    if (offsetOldY) dAccess<float>(&sp, offsetOldY) = y;
    
}

// fix fall damage - teleport command or LL teleport API
TInstanceHook(
    void,
    "?teleportTo@Player@@UEAAXAEBVVec3@@_NHH@Z",
    ServerPlayer,
    class Vec3 const& pos,
    bool              shouldStopRiding,
    int               cause,
    int               sourceEntityType
) {
    if (lfp::utils::sp_utils::isFakePlayer(this)) {
#ifdef LFP_DEBUG
        auto& vs = *(voids<1200>*)this;
        LOG_VAR(getPos().toString());
        DEBUGL(
            "Player({})::teleportTo({}, {}, {}, {})",
            this->getNameTag(),
            pos.toString(),
            shouldStopRiding,
            cause,
            sourceEntityType
        );
        LOG_VAR(this->isOnGround());
        LOG_VAR(dAccess<float>(this, 9408));
        LOG_VAR((int)dAccess<char>(this, 4216));
        LOG_VAR(this->hasTeleported());
#endif // LFP_DEBUG
        trySetOldY(*(SimulatedPlayer*)this, pos.y);
    }
    origin( pos, shouldStopRiding, cause, sourceEntityType);
}

// fix fall damage - enter end_portal in the end
TInstanceHook(
    void,
    "?changeDimensionWithCredits@ServerPlayer@@UEAAXV?$AutomaticID@VDimension@@H@@@Z",
    ServerPlayer,
    class AutomaticID<class Dimension, int> dimid
) {
    if (lfp::utils::sp_utils::isFakePlayer(this)) {
#ifdef LFP_DEBUG
        LOG_VAR(getPos().toString());
        DEBUGL("ServerPlayer({})::changeDimensionWithCredits({})", getName(), dimid);
#endif // LFP_DEBUG
        trySetOldY(*(SimulatedPlayer*)this, FLT_MAX);
    }
    origin( dimid);
}

class ChangeDimensionRequest {
public:
    int                          mState;
    AutomaticID<Dimension, int>  mFromDimensionId;
    AutomaticID<Dimension, int>  mToDimensionId;
    Vec3                         mPosition;
    bool                         mUsePortal;
    bool                         mRespawn;
    std::unique_ptr<CompoundTag> mAgentTag;
    std::string                  toDebugString() {
        return fmt::format(
            "state: {}, dim: {}->{}, position: ({}), usePortal: {}, respawn: {}",
            mState,
            (int)mFromDimensionId,
            (int)mToDimensionId,
            mPosition.toString(),
            mUsePortal,
            mRespawn
        );
    }
};

// fix fall damage - teleport between dimensions by teleport command or LL teleport API
TInstanceHook(
    void,
    "?requestPlayerChangeDimension@Level@@UEAAXAEAVPlayer@@V?$unique_ptr@VChangeDimensionRequest@@"
    "U?$default_delete@VChangeDimensionRequest@@@std@@@std@@@Z",
    Level,
    Player&                                 player,
    std::unique_ptr<ChangeDimensionRequest> requestPtr
) {
    DEBUGL("Level::requestPlayerChangeDimension({}, requestPtr)", player.getNameTag());
    DEBUGL("Request: {}", packetToString(*requestPtr));
    // DEBUGL(getPlayerStateString(&player));
    if (lfp::utils::sp_utils::isFakePlayer(player)) trySetOldY((SimulatedPlayer&)player, FLT_MAX);
    return origin( player, std::move(requestPtr));
}
#endif

#include "mc/network/LoopbackPacketSender.h"


#if false
#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/gameplayhandlers/ActorGameplayHandler.h"
#include "mc/gameplayhandlers/PlayerGameplayHandler.h"
#include "mc/gameplayhandlers/ServerNetworkEventHandler.h"
#include "mc/world/events/ActorEventCoordinator.h"
#include "mc/world/events/ActorGameplayEvent.h"
#include "mc/world/events/PlayerEventCoordinator.h"
#include "mc/world/events/PlayerGameplayEvent.h"
#include "mc/world/events/ServerNetworkEventCoordinator.h"
#include "mc/world/events/ServerPlayerEventCoordinator.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/ItemStackDescriptor.h"
#include "mc/world/level/LevelEventManager.h"


class SimulatedEventHandler : public ActorGameplayHandler {
public:
    SimulatedEventHandler(const SimulatedEventHandler&)            = default;
    SimulatedEventHandler(SimulatedEventHandler&&)                 = default;
    SimulatedEventHandler& operator=(const SimulatedEventHandler&) = default;
    SimulatedEventHandler& operator=(SimulatedEventHandler&&)      = default;

    virtual ~SimulatedEventHandler() /*override*/ = default;
    virtual ::HandlerResult handleEvent(::ActorGameplayEvent<void> const& event) {
        return event.visit([](const auto& ev) {
            using T = std::decay_t<decltype(ev)>;
            if constexpr (std::is_same_v<T, ActorCarriedItemChangedEvent>) {
                auto en = ev.mEntity->tryUnwrap();
                if (en.has_value() && lfp::utils::sp_utils::isFakePlayer(*en)) {
                    MobEquipmentPacket pkt(
                        en->getRuntimeID(),
                        en->getCarriedItem(),
                        (int)ev.mHandSlot,
                        (int)ev.mHandSlot,
                        ContainerID::Inventory
                    );
                    SimulatedClient::send((SimulatedPlayer&)*en, pkt);
                }
            }
            return ::HandlerResult::BypassListeners;
        });
    };

    // vIndex: 2
    virtual ::GameplayHandlerResult<::CoordinatorResult>
    handleEvent(::ActorGameplayEvent<::CoordinatorResult> const& event) {
        return ::GameplayHandlerResult<::CoordinatorResult>{};
    };

    // vIndex: 1
    virtual ::GameplayHandlerResult<::CoordinatorResult>
    handleEvent(::MutableActorGameplayEvent<::CoordinatorResult>& event) {
        return ::GameplayHandlerResult<::CoordinatorResult>{};
    };
};

bool subscribeEvent(SimulatedPlayer& sp) {
    auto& level =(ServerLevel&) *ll::service::getLevel();
    level.getServerNetworkEventCoordinator().sendEvent(std::make_unique<SimulatedEventHandler>());
}

#endif

// fix carried item display


// TClasslessInstanceHook(
//     void,
//     "?sendActorCarriedItemChanged@ActorEventCoordinator@@QEAAXAEAVActor@@AEBVItemInstance@@"
//     "1W4HandSlot@@@Z",
//     class Actor&              actor,
//     class ItemInstance const& oldItem,
//     class ItemInstance const& newItem,
//     enum HandSlot             slot
// ) {
//     origin(actor, oldItem, newItem, slot);

//     if (lfp::utils::sp_utils::isFakePlayer(actor)) {
//         // Force to call the implementation of ServerPlayer
//         MobEquipmentPacket
//             pkt(actor.getRuntimeID(), newItem, (int)slot, (int)slot, ContainerID::Inventory);
//         SimulatedClient::send((SimulatedPlayer*)&actor, pkt);
//     }
// }
#if false

// fix armor display
TClasslessInstanceHook(
    void,
    "?sendActorEquippedArmor@ActorEventCoordinator@@QEAAXAEAVActor@@AEBVItemInstance@@W4ArmorSlot@@"
    "@Z",
    class Actor&              actor,
    class ItemInstance const& item,
    enum ArmorSlot            slot
) {
    origin( actor, item, slot);

    if (lfp::utils::sp_utils::isFakePlayer(actor)) {
        MobEquipmentPacket
            pkt(actor.getRuntimeID(), item, (int)slot, (int)slot, ContainerID::Armor);
        SimulatedClient::send((SimulatedPlayer*)&actor, pkt);
    }
}

#endif

#ifdef LFP_DEBUG

// ================= Test =================

// Player offset:
//?3945  bool mIsInitialSpawnDone
// 2240  bool mServerHasMovementAuthority
// 553*4 int  mDimensionState - ServerPlayer::isPlayerInitialized
// 3929  bool mRespawnReady - ServerPlayer::isPlayerInitialized
// 8936  bool mLoading - ServerPlayer::isPlayerInitialized
// 8938  bool mLocalPlayerInitialized - ServerPlayer::setLocalPlayerAsInitialized
// 8488  NetworkChunkPublisher

std::string getPlayerStateString(SimulatedPlayer* player) {
    std::string str  = "\r";
    str             += fmt::format(
        "\nposition: {}({})",
        VanillaDimensions::toString(player->getDimensionId()),
        player->getPosition().toString()
    );
    str += fmt::format("\nmIsInitialSpawnDone: {}", player->mIsInitialSpawnDone);
    str += fmt::format(
        "\nmServerHasMovementAuthority: {}",
        magic_enum::enum_name(player->mMovementSettings->AuthorityMode)
    );
    str += fmt::format(
        "\nmSpawnPositionState: {}",
        magic_enum::enum_name(player->mSpawnPositionState)
    );
    str += fmt::format("\nmRespawnReady: {}", player->mRespawnReady);
    str += fmt::format("\nmForcedLoading: {}", player->mForcedLoading);
    str += fmt::format("\nmLocalPlayerInitialized: {}", player->mIsInitialSpawnDone);

    // str += fmt::format("\nmIsInitialSpawnDone: {}", dAccess<bool>(player, 3945));
    // str += fmt::format("\nmServerHasMovementAuthority: {}", dAccess<bool>(player, 2240));
    // str += fmt::format("\nmDimensionState: {}", dAccess<int>(player, 559 * 4));
    // str += fmt::format("\nmRespawnReady: {}", dAccess<bool>(player, 3904));
    // str += fmt::format("\nmLoading: {}", dAccess<bool>(player, 8952));
    // str += fmt::format("\nmLocalPlayerInitialized: {}", dAccess<bool>(player, 8954));
    return str;
}


std::pair<std::string, int>
genTickingInfo(::BlockSource const& region, BlockPos const& bpos, int range);


LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_Level_$requestPlayerChangeDimension,
    ll::memory::HookPriority::Normal,
    Level,
    &ServerLevel::$requestPlayerChangeDimension,
    void,
    ::Player&                  player,
    ::ChangeDimensionRequest&& changeRequest
) {
    origin(player, std::move(changeRequest));
    if (!lfp::utils::sp_utils::isFakePlayer(&player)) return;
    static size_t index = 0;
    bool          rtn   = true;
    if (rtn || (index++ % 20 == 0)) {
        if (rtn) index = 0;
        auto bpos    = player.getFeetBlockPos();
        int  range   = 32 * 2 >> 4; // IDA Level::_playerChangeDimension line 339 before return
        int  chunk_x = bpos.x >> 4;
        int  chunk_z = bpos.z >> 4;
        auto max_cx  = chunk_x + range;
        auto min_cx  = chunk_x - range;
        auto max_cz  = chunk_z + range;
        auto min_cz  = chunk_z - range;
        auto totalChunksCount = (max_cx - min_cx + 1) * (max_cz - min_cz + 1);

        auto& region = player.getDimensionBlockSource();
        auto& cs     = region.getChunkSource();

        auto loadInfo = lfp::genTickingInfo(region, bpos, range);
        DEBUGL(
            "Dimension: {}, cx range:[{}:{}], cz range:[{}:{}], Info: {}",
            (int)region.getDimensionId(),
            min_cx,
            max_cx,
            min_cz,
            max_cz,
            lfp::utils::color_utils::convertToConsole(loadInfo.first)
        );
        DEBUGL("Loaded Chunk: total: {}, loaded:{} ", totalChunksCount, loadInfo.second);
    }
    return;
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_$prepareRegion,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::$prepareRegion,
    void,
    ::ChunkSource& mainChunkSource
) {
    // if (lfp::utils::sp_utils::isFakePlayer(this) && !&getDimension()) {
    //     auto dim = ll::service::getLevel()->getOrCreateDimension(this->getDimensionId()).lock();
    //     if (dim.get()) return origin(*dim->mChunkSource);
    // }
    DEBUGL(
        "ServerPlayer({})::$prepareRegion(ChunkSource(dimid: {}))",
        this->getNameTag(),
        (int)mainChunkSource.mDimension->getDimensionId()
    );

    origin(mainChunkSource);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_$destroyRegion,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::$destroyRegion,
    void,
) {
    DEBUGL("ServerPlayer({})::$destroyRegion()", this->getNameTag());
    return origin();
}

// LL_AUTO_TYPE_INSTANCE_HOOK(
//     LFP_ServerPlayer_resendAllChunks,
//     ll::memory::HookPriority::Normal,
//     ServerPlayer,
//     &ServerPlayer::resendAllChunks,
//     void,
// ) {
//     DEBUGL("ServerPlayer({})::resendAllChunks()", this->getNameTag());
//     return origin();
// }

LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_$_fireDimensionChanged,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::$_fireDimensionChanged,
    void,
) {
    DEBUGL("ServerPlayer({})::$_fireDimensionChanged()", this->getNameTag());
    return origin();
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_$suspendRegion,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::$suspendRegion,
    void,
) {
    DEBUGL("ServerPlayer({})::$suspendRegion()", this->getNameTag());
    return origin();
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_fireDimensionChangedEvent,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::fireDimensionChangedEvent,
    void,
    ::DimensionType fromDimension,
    ::DimensionType toDimension
) {

    DEBUGL(
        "Player({})::fireDimensionChangedEvent({}, {})",
        this->getNameTag(),
        (int)fromDimension,
        (int)toDimension
    );
    return origin(fromDimension, toDimension);
}


LL_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_$_updateChunkPublisherView,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::$_updateChunkPublisherView,
    void,
    ::Vec3 const& position,
    float         minDistance
) {
    static __int64 logTick = 0;
    if (++logTick % 20 == 0)
        DEBUGL(
            "Player({})::$_updateChunkPublisherView(({}), {})",
            this->getNameTag(),
            position.toString(),
            minDistance
        );
    return origin(position, minDistance);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_$_getSpawnChunkLimit,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::$_getSpawnChunkLimit,
    int,
) {
    static __int64 logTick = 0;
    int            rtn     = origin();
    if (++logTick % 20 == 0)
        DEBUGL("ServerPlayer({})::$_getSpawnChunkLimit() -> {}", this->getNameTag(), rtn);
    return rtn;
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_$respawn,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &SimulatedPlayer::$respawn,
    void,
) {
    DEBUGL("ServerPlayer({})::$respawn()", this->getNameTag());
    return origin();
}


LL_AUTO_TYPE_INSTANCE_HOOK(
    LFP_ServerPlayer_$die,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &SimulatedPlayer::$die,
    void,
    ::ActorDamageSource const& source
) {
    DEBUGW("ServerPlayer({})::$die()", this->getNameTag());
    if (lfp::utils::sp_utils::isFakePlayer(this)) {
        LOG_VAR(this->isOnGround());
        // LOG_VAR(dAccess<float>(this, 468));
        // LOG_VAR((int)dAccess<char>(this, 4216));
    }
    origin(source);
}

// LL_AUTO_TYPE_INSTANCE_HOOK(
//     LFP_ServerPlayer_travel,
//     ll::memory::HookPriority::Normal,
//     ServerPlayer,
//     &ServerPlayer::travel,
//     void,
//     ::ActorDamageSource const& source
// ) {
//     &ServerPlayer::_moveRelative(::Vec3 &posDelta, float yRotDegrees, float xa, float ya,
//     float za, float speed),

//     DEBUGL("ServerPlayer({})::travel({}, {}, {})", this->getNameTag(), x, y, z);
//     if (lfp::utils::sp_utils::isFakePlayer(this)) {
//         LOG_VAR(this->isOnGround());
//         // LOG_VAR(this->mhast());
//         // LOG_VAR(dAccess<float>(this, 468));
//         // LOG_VAR((int)dAccess<char>(this, 4216));
//     }
//     origin(x, y, z);
// }


// TInstanceHook(void, "?checkFallDamage@ServerPlayer@@UEAAXM_N@Z",
//               ServerPlayer, float unkFloat, bool unkBool)
//{
//     if (unkFloat > 0)
//         DEBUGL("ServerPlayer({})::checkFallDamage({}, {})", getName(), unkFloat, unkBool);
//     origin( unkFloat, unkBool);
// }

// TInstanceHook(
//     void,
//     "?handleFallDistanceOnServer@ServerPlayer@@UEAAXMM_N@Z",
//     ServerPlayer,
//     float unkFloat,
//     float unkFloat2,
//     bool  unkBool
// ) {
//     DEBUGW(
//         "ServerPlayer({})::handleFallDistanceOnServer({}, {}, {})",
//         getName(),
//         unkFloat,
//         unkFloat2,
//         unkBool
//     );
//     origin(unkFloat, unkFloat2, unkBool);
// }
// #include <MC/ActorDamageSource.hpp>

// TInstanceHook(
//     void,
//     "?causeFallDamage@Actor@@UEAAXMMVActorDamageSource@@@Z",
//     ServerPlayer,
//     float                    unkFloat,
//     float                    unkFloat2,
//     class ActorDamageSource& source
// ) {
//     DEBUGW(
//         "ServerPlayer({})::causeFallDamage({}, {}, {})",
//         getName(),
//         unkFloat,
//         unkFloat2,
//         magic_enum::enum_name(source.getCause())
//     );
//     origin(unkFloat, unkFloat2, source);
// }

// TClasslessInstanceHook(void,
// "?setFallDistance@?$DirectActorProxyImpl@UIPlayerMovementProxy@@@@UEAAXM@Z",
//                        float fallDistance)
//{
//     DEBUGW("DirectActorProxyImpl::setFallDistance({})", fallDistance);
//     origin( fallDistance);
// }

#endif // LFP_DEBUG

#endif