#pragma once

#ifdef LFP_DEBUG
#include <memory>
#include <string_view>

#include "ll/api/memory/Hook.h"
#include "magic_enum.hpp"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/game_refs/GameRefs.h"
#include "mc/world/phys/AABB.h"

#include "ll/api/service/Bedrock.h"
#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/resource/PackIdVersion.h"
#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/deps/game_refs/StackRefResult.h"
#include "mc/deps/identity/edu_common/MessToken.h"
#include "mc/entity/components/ActorUniqueIDComponent.h"
#include "mc/entity/components_json_legacy/NpcComponent.h"
#include "mc/gametest/MinecraftGameTestHelper.h"
#include "mc/gametest/NullGameTestHelper.h"
#include "mc/gametest/framework/BaseGameTestHelper.h"
#include "mc/gametest/framework/GameTestError.h"
#include "mc/gametest/framework/tags.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/Int64Tag.h"
#include "mc/network/NetworkServerConfig.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/ServerNetworkSystem.h"
#include "mc/network/connection/DisconnectFailReason.h"
#include "mc/network/packet/NpcRequestPacket.h"
#include "mc/network/packet/Packet.h"
#include "mc/platform/UUID.h"
#include "mc/scripting/modules/gametest/ScriptGameTestConnectivity.h"
#include "mc/scripting/modules/gametest/ScriptNavigationResult.h"
#include "mc/scripting/modules/gametest/ScriptSimulatedPlayer.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/Minecraft.h"
#include "mc/world/actor/ActorDamageSource.h"
#include "mc/world/actor/ai/control/BodyControl.h"
#include "mc/world/actor/npc/ActionContainer.h"
#include "mc/world/actor/npc/CommandAction.h"
#include "mc/world/actor/npc/StoredCommand.h"
#include "mc/world/actor/npc/UrlAction.h"
#include "mc/world/actor/npc/npc.h"
#include "mc/world/inventory/transaction/ComplexInventoryTransaction.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/GameplayUserManager.h"
#include <atomic>
#include <string>
#include <utility>


#include "lfp/utils/packets.h"

template <typename T>
using GameTestResultVariant = ::std::variant<::gametest::GameTestError, T>;

template <>
struct GameRefs<EntityContext> {
    using OwnerStorage       = std::shared_ptr<EntityContext>;
    using StackResultStorage = std::shared_ptr<EntityContext>;
    using WeakStorage        = std::weak_ptr<EntityContext>;
    using StackRef           = EntityContext;
};

template <typename T, bool LOG = true>
inline void onHookCall(std::string_view const name) {
    if constexpr (LOG) {
        fmt::println("{} called", name);
    }
}

template <typename T, bool LOG = true, typename Pkt>
    requires std::is_base_of_v<Packet, Pkt>
void onHandlePacket(NetworkIdentifier const&, Pkt const& packet) {
    if constexpr (LOG) {
        fmt::
            println("Handle packet<{:3}> {}", (int)packet.getId(), ll::reflection::type_name_v<Pkt>);
    }
}
template <typename T, bool LOG = true, typename Pkt>
    requires std::is_base_of_v<Packet, Pkt>
void onHandlePacket(NetworkIdentifier const&, std::shared_ptr<Pkt> packet) {
    if constexpr (LOG) {
        fmt::
            println("Handle packet<{:3}> {}", (int)packet->getId(), ll::reflection::type_name_v<Pkt>);
    }
}


#define HOOK_Player
#define HOOK_ServerPlayer
#define HOOK_SimulatedPlayer
#define HOOK_ServerNetworkHandler
// #define HOOK_MinecraftGameTestHelper
#define HOOK_LoopbackPacketSender
// #define HOOK_PacketObserver
#define HOOK_PlayerContainerSetter

#endif
