#pragma once

#include <cassert>
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "mc/_HeaderOutputPredefine.h"
#include "mc/common/SubClientId.h"
#include "mc/gametest/framework/BaseGameTestHelper.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/level/BlockPos.h"

#include "lfp/manager/FakePlayerManager.h"

namespace lfp::fix {
struct PlayerIdsFix;
struct SpawnFix_respawnPos;
} // namespace lfp::fix

namespace lfp::inline manager {

class FakePlayerManager;
class FakePlayerStorage;


class FakePlayer {
private:
    FakePlayerManager&  mManager;
    std::string const   mRealName;
    mce::UUID const     mUuid;
    ActorUniqueID const mUniqueId;
    time_t              mLastOnlineTime;
    bool                mAutoLogin = false;

    // Online Data
    std::string      mXuid;
    bool             mOnline      = false;
    SimulatedPlayer* mPlayer      = nullptr;
    SubClientId      mSenderSubId = static_cast<SubClientId>(-1);

    // Temperate data
    std::unique_ptr<CompoundTag>                      mCachedTag = {};
    std::optional<std::pair<BlockPos, DimensionType>> mCreateAt;

    static FakePlayer*   sLoggingInPlayer;
    static unsigned char sMaxClientSubId;

    [[nodiscard]] static SubClientId getNextClientSubId();
    // [[nodiscard]] std::string        getServerId() const;
    // [[nodiscard]] std::string        getStorageId() const;

    friend class lfp::FakePlayerManager;
    friend class lfp::FakePlayerStorage;
    friend struct lfp::fix::PlayerIdsFix;
    friend struct lfp::fix::SpawnFix_respawnPos;
    friend struct lfp::fix::LoadPlayerDataFix;

    void setDefaultSpawn(BlockPos const& spawnPos, DimensionType dimension);

public:
    static NetworkIdentifier const& FAKE_NETWORK_ID;
    FakePlayer(
        FakePlayerManager& manager,
        std::string        realName,
        mce::UUID          uuid,
        ActorUniqueID      uniqueId,
        time_t             lastOnlineTime = 0,
        bool               autoLogin      = false
    );

    ~FakePlayer();

    [[nodiscard]] static std::unique_ptr<FakePlayer>
    deserialize(CompoundTag const& tag, FakePlayerManager* manager = nullptr);
    [[nodiscard]] std::unique_ptr<CompoundTag> serialize() const;
    bool                                       serialize(CompoundTag& tag) const;

    bool login();
    bool logout(bool save = true);

    [[nodiscard]] std::string                getXuid() const;
    [[nodiscard]] mce::UUID const&           getUuid() const;
    [[nodiscard]] constexpr ActorUniqueID    getUniqueId() const { return mUniqueId; };
    [[nodiscard]] constexpr SubClientId      getClientSubId() const { return mSenderSubId; }
    [[nodiscard]] constexpr std::string      getRealName() const { return mRealName; }
    [[nodiscard]] constexpr SimulatedPlayer* getRuntimePlayer() const { return mPlayer; };
    [[nodiscard]] constexpr bool             isOnline() const { return mOnline; }
    [[nodiscard]] constexpr bool             shouldAutoLogin() const { return mAutoLogin; }
    [[nodiscard]] constexpr time_t           lastOnlineTimeStamp() const { return mLastOnlineTime; }

    [[nodiscard]] std::unique_ptr<CompoundTag> getPlayerData() const;
    [[nodiscard]] std::unique_ptr<CompoundTag> getSavedPlayerData() const;
    [[nodiscard]] std::unique_ptr<CompoundTag> getRuntimePlayerData() const;
};

} // namespace lfp::inline manager