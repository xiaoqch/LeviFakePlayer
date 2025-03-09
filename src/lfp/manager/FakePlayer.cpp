#include "FakePlayer.h"

#include <cassert>
#include <memory>
#include <string>

#include "mc/common/SubClientId.h"
#include "mc/dataloadhelper/DefaultDataLoadHelper.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/entity/components/ActorUniqueIDComponent.h"
#include "mc/gametest/MinecraftGameTestHelper.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/Int64Tag.h"
#include "mc/nbt/ListTag.h"
#include "mc/nbt/Tag.h"
#include "mc/platform/UUID.h"
#include "mc/scripting/modules/gametest/ScriptSimulatedPlayer.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/Minecraft.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/storage/LevelData.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/service/Bedrock.h"

#include "lfp/LeviFakePlayer.h"
#include "lfp/api/events/FakePlayerJoinEvent.h"
#include "lfp/api/events/FakePlayerLeaveEvent.h"
#include "lfp/fix/FixManager.h"
#include "lfp/manager/FakePlayerManager.h"
#include "lfp/utils/DebugUtils.h"
#include "lfp/utils/NbtUtils.h"
#include "lfp/utils/SimulatedPlayerUtils.h"


namespace lfp::inline manager {

FakePlayer*       FakePlayer::mLoggingInPlayer = nullptr;
NetworkIdentifier FakePlayer::FAKE_NETWORK_ID(NetworkIdentifier::INVALID_ID());

constexpr SubClientId INVALID_SUB_CLIENT_ID = SubClientId::PrimaryClient;


SubClientId FakePlayer::getNextClientSubId() {
    static unsigned char currentCliendSubId = 1 + static_cast<unsigned char>(SubClientId::Client4);
    if (currentCliendSubId == 255) {
        currentCliendSubId = static_cast<unsigned char>(SubClientId::Client4);
        FakePlayerManager::getManager().mLogger.warn("clientSubId overflow");
    }
    return static_cast<SubClientId>(currentCliendSubId++);
}

FakePlayer::FakePlayer(
    FakePlayerManager& manager,
    std::string        realName,
    mce::UUID          uuid,
    time_t             lastOnlineTime,
    bool               autoLogin
)
: mManager(manager),
  mRealName(std::move(realName)),
  mUuid(uuid),
  mLastOnlineTime(lastOnlineTime),
  mAutoLogin(autoLogin) {
    mClientSubId = INVALID_SUB_CLIENT_ID;
    mXuid        = lfp::utils::sp_utils::xuidFromUuid(mUuid);
    DEBUGL(
        "FakePlayer::FakePlayer({}, {}, {}, {}",
        realName,
        uuid.asString(),
        lastOnlineTime,
        autoLogin
    );
}

FakePlayer::~FakePlayer() { DEBUGL("FakePlayer::~FakePlayer() - {}", mRealName); }

bool FakePlayer::serialize(CompoundTag& tag) const {
    tag["realName"]       = mRealName;
    tag["uuid"]           = mUuid.asString();
    tag["lastOnlineTime"] = mLastOnlineTime;
    tag["autoLogin"]      = mAutoLogin;
    return true;
};


std::unique_ptr<FakePlayer>
FakePlayer::deserialize(CompoundTag const& tag, FakePlayerManager* manager) {
    try {
        if (manager == nullptr) {
            manager = &lfp::FakePlayerManager::getManager();
        }
        std::string name           = tag["realName"];
        std::string suuid          = tag["uuid"];
        auto        uuid           = mce::UUID::fromString(suuid);
        time_t      lastOnlineTime = tag["lastOnlineTime"].get<Int64Tag>();
        bool        autoLogin      = tag["autoLogin"].get<ByteTag>();
        if (name.empty() || !uuid) {
            auto& logger = LeviFakePlayer::getLogger();
            logger.info("FakePlayer Data Error, name: {}, uuid: {}", name, suuid);
            return {};
        }
        return std::make_unique<FakePlayer>(*manager, name, uuid, lastOnlineTime, autoLogin);
    } catch (...) {
        auto& logger = LeviFakePlayer::getLogger();
        logger.error("Error in " __FUNCTION__);
    }
    return {};
}

std::unique_ptr<CompoundTag> FakePlayer::serialize() const {
    auto tag = std::make_unique<CompoundTag>();
    if (serialize(*tag)) return tag;
    return {};
}


bool FakePlayer::login() {
    if (mOnline || mLoggingInPlayer) return false;
    mLoggingInPlayer = this;
    if (mClientSubId == SubClientId::PrimaryClient) {
        mClientSubId = getNextClientSubId();
    }
    LeviFakePlayer::getInstance().getFixManager().beforeFakePlayerLogin();
    mCachedTag = mManager.loadPlayerData(*this);

    // TODO: Fix for FakePlayerSwapTest
    auto TEMP_FIX_DataBeforeLogin =
        mCachedTag ? mCachedTag->clone() : std::unique_ptr<CompoundTag>();

    auto dim  = ll::service::getLevel()->getLastOrDefaultSpawnDimensionId(3);
    auto bpos = ll::service::getLevel()->asServer().getLevelData().getSpawnPos();
    auto uid  = ActorUniqueID::INVALID_ID();
    try {
        if (mCachedTag) {
            auto& tag = *mCachedTag;
            auto  pos = utils::nbt_utils::toVec3(tag["Pos"].get<ListTag>());
            bpos      = {pos.x, pos.y - 1.60001f, pos.z};
            dim       = (int)tag["DimensionId"];
            uid       = (ActorUniqueID)tag["UniqueID"].get<Int64Tag>();
        } else if (mCreateAt) {
            bpos = mCreateAt->first;
            dim  = mCreateAt->second;
        }
        mUniqueId = uid;
        mPlayer   = lfp::utils::sp_utils::create(mRealName, bpos, dim, mXuid);
#ifdef LFP_DEBUG
        auto oldData = getSavedPlayerData();
        if (oldData) {
            auto newData = getRuntimePlayerData();
            auto [o, n]  = lfp::utils::nbt_utils::tagDiff(*oldData, *newData, 1);
            mManager.mLogger.debug(
                "Tag Difference:\n{}\n{}",
                o ? o->toSnbt(SnbtFormat::PrettyConsolePrint) : "",
                n ? n->toSnbt(SnbtFormat::PrettyConsolePrint) : ""
            );
        }
        // else {
        //     mManager.mLogger.debug(
        //         "New data:\n{}",
        //         newData->toSnbt(SnbtFormat::PrettyConsolePrint)
        //     );
        // }
#endif
    } catch (...) {
        mLoggingInPlayer = nullptr;
        mCachedTag.reset();
        throw;
    }
    mLoggingInPlayer = nullptr;

    if (!mPlayer) return false;

    if (TEMP_FIX_DataBeforeLogin) {
        DefaultDataLoadHelper ddlh{};
        mPlayer->load(*TEMP_FIX_DataBeforeLogin, ddlh);
    }
    mUniqueId    = mPlayer->getOrCreateUniqueID();
    mClientSubId = mPlayer->getClientSubId();
    time(&mLastOnlineTime);

    mOnline = true;
    lfp::event::FakePlayerJoinEvent ev(*this);
    ll::event::EventBus::getInstance().publish(ev);
    mManager.saveRuntimeData(*this);
    return true;
};

bool FakePlayer::logout(bool save) {
    if (!mOnline || !mPlayer) return false;
    if (save) {
        time(&mLastOnlineTime);
        mManager.savePlayer(*this);
    }
    lfp::event::FakePlayerLeaveEvent ev(*this);
    ll::event::EventBus::getInstance().publish(ev);
    ((MinecraftGameTestHelper*)nullptr)->$removeSimulatedPlayer(*mPlayer);

    if (nullptr != ServerPlayer::tryGetFromEntity(mPlayer->getEntityContext(), false)) {
        mManager.mLogger.debug(
            "Logout {} failed, Unexpected player object got from level.",
            mRealName
        );
        return false;
    }
    mOnline = false;
    mPlayer = nullptr;
    LeviFakePlayer::getInstance().getFixManager().afterFakePlayerLogout();
    // mClientSubID = -1;
    return true;
}


mce::UUID const& FakePlayer::getUuid() const { return mUuid; }

std::string FakePlayer::getXuid() const { return mXuid; };

std::unique_ptr<CompoundTag> FakePlayer::getPlayerData() const {
    if (!mOnline || !mPlayer) {
        return getSavedPlayerData();
    }
    return getRuntimePlayerData();
}

std::unique_ptr<CompoundTag> FakePlayer::getSavedPlayerData() const {
    return mManager.loadPlayerData(*this);
}

std::unique_ptr<CompoundTag> FakePlayer::getRuntimePlayerData() const {
    if (!mPlayer) return nullptr;
    auto tag = std::make_unique<CompoundTag>();
    mPlayer->save(*tag);
    return tag;
}

void FakePlayer::setDefaultSpawn(BlockPos const& spawnPos, DimensionType dimension) {
    mCreateAt = {spawnPos, dimension};
}
} // namespace lfp::inline manager