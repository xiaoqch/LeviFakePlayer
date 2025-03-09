#include "FakePlayerManager.h"

#include <cassert>
#include <ctime>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>

#include "mc/dataloadhelper/DefaultDataLoadHelper.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/platform/UUID.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/storage/DBStorage.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/io/Logger.h"
#include "ll/api/service/Bedrock.h"

#include "lfp/LeviFakePlayer.h"
#include "lfp/api/events/FakePlayerCreatedEvent.h"
#include "lfp/api/events/FakePlayerRemoveEvent.h"
#include "lfp/api/events/FakePlayerUpdateEvent.h"
#include "lfp/manager/FakePlayer.h"
#include "lfp/manager/FakePlayerStorage.h"
#include "lfp/utils/DebugUtils.h"
#include "lfp/utils/NbtUtils.h"
#include "lfp/utils/SimulatedPlayerUtils.h"


namespace lfp::inline manager {


FakePlayerManager::FakePlayerManager(std::filesystem::path const& dbPath)
: mStorage(std::make_unique<FakePlayerStorage>(dbPath)),
  mLogger(lfp::LeviFakePlayer::getLogger()) {
    DEBUGL("FakePlayerManager::FakePlayerManager({})", dbPath);
    lfp::FakePlayer::FAKE_NETWORK_ID.mType = ::NetworkIdentifier::Type::Invalid;

    this->initFakePlayers();
}

void FakePlayerManager::initFakePlayers() {

    // std::vector<std::tuple<time_t, std::string_view>> tmp;
    for (auto [uuid, tag] : mStorage->iter()) {
        auto fp                              = FakePlayer::deserialize(*tag, this);
        mLookupMap[fp->getRealName()]        = fp.get();
        mLookupMap[fp->getUuid().asString()] = fp.get();
        mLookupMap[fp->getXuid()]            = fp.get();
        // tmp.push_back({fp->mLastOnlineTime, fp->mRealName});
        mFakePlayers[uuid] = std::move(fp);
    }
}

FakePlayerManager::~FakePlayerManager() { DEBUGW("FakePlayerManager::~FakePlayerManager()"); }

FakePlayerManager& FakePlayerManager::getManager() {
    return lfp::LeviFakePlayer::getInstance().getManager();
}

std::unique_ptr<CompoundTag> FakePlayerManager::loadPlayerData(lfp::FakePlayer const& fp) {
    return mStorage->loadPlayerTag(fp.getUuid());
};

bool FakePlayerManager::saveFakePlayerInfo(FakePlayer& fakePlayer) {
    auto uuid = fakePlayer.getUuid();
    if (fakePlayer.isOnline()) {
        time(&fakePlayer.mLastOnlineTime);
    }
    return mStorage->savePlayerInfo(uuid, fakePlayer.serialize());
}

bool FakePlayerManager::saveRuntimeData(FakePlayer& fakePlayer) {
    auto uuid = fakePlayer.getUuid();
    if (fakePlayer.isOnline()) {
        return mStorage->savePlayerTag(uuid, fakePlayer.getRuntimePlayerData());
    }
    return false;
}

bool FakePlayerManager::savePlayer(FakePlayer& fp) {
    if (!fp.isOnline()) return saveFakePlayerInfo(fp);
    return saveFakePlayerInfo(fp) && saveRuntimeData(fp);
}

int FakePlayerManager::savePlayers(bool onlineOnly) {
    auto count = 0;
    for (auto& [uuid, player] : mFakePlayers) {
        if (!onlineOnly) {
            saveFakePlayerInfo(*player);
        }
        if (player->isOnline() && !saveRuntimeData(*player)) ++count;
    }
    return count;
}

bool FakePlayerManager::importData_JavaFakePlayer(std::string const& name) {
    auto& logger    = lfp::LeviFakePlayer::getLogger();
    auto  uuid      = lfp::utils::sp_utils::JAVA_nameUUIDFromBytes(name);
    auto  suuid     = uuid.asString();
    auto& dbStorage = ll::service::getLevel()->getLevelStorage();
    auto  tag = dbStorage.getCompoundTag("player_" + uuid.asString(), DBHelpers::Category::Player);
    if (!tag) {
        logger.error("Error in getting PlayerStorageIds");
        return false;
    }
    auto& serverId = (*tag)["ServerId"].get<StringTag>();
    if (serverId.empty()) {
        logger.error("Error in getting player's ServerId");
        return false;
    }
    auto playerData = dbStorage.getCompoundTag(serverId, DBHelpers::Category::Player);
    auto fp         = create(name, false, std::move(playerData));
    if (fp) return true;
    logger.error("Error in reading player's data");
    return false;
}

lfp::FakePlayer*
FakePlayerManager::create(std::string name, bool login, std::unique_ptr<CompoundTag> playerData) {
    if (tryGetFakePlayer(name)) return {};
    auto uuid = utils::sp_utils::uuidFromName(name);
    auto tmp  = std::make_unique<FakePlayer>(*this, name, uuid, time(0), false);
    auto fp   = mFakePlayers.emplace(uuid, std::move(tmp)).first->second.get();
    mLookupMap.try_emplace(name, fp);
    mLookupMap.try_emplace(fp->getXuid(), fp);
    mLookupMap.try_emplace(uuid.asString(), fp);
    // mSortedNames.push_back(name);
    saveFakePlayerInfo(*fp);
    if (playerData && !mStorage->savePlayerTag(uuid, *playerData)) {
        mLogger.error("Failed to create fake player with data");
        return {};
    }
    lfp::event::FakePlayerCreatedEvent ev(*fp);
    ll::event::EventBus::getInstance().publish(ev);
    if (login) fp->login();
    return fp;
}

FakePlayer*
FakePlayerManager::create(std::string name, BlockPos const& spawnPos, DimensionType dimension) {
    auto fp = create(std::move(name), false);
    if (fp) {
        fp->setDefaultSpawn(spawnPos, dimension);
        fp->login();
    }
    return fp;
}

bool FakePlayerManager::remove(lfp::FakePlayer& fp) {
    if (fp.isOnline()) fp.logout(false);
    auto                              uuid = fp.getUuid();
    auto                              name = fp.getRealName();
    auto                              xuid = fp.getXuid();
    lfp::event::FakePlayerRemoveEvent ev(fp);
    ll::event::EventBus::getInstance().publish(ev);
    if (!mStorage->removePlayerData(uuid)) {
        mLogger.error("Failed to remove data of {}", name);
        return false;
    }
    mFakePlayers.erase(uuid);
    mLookupMap.erase(name);
    mLookupMap.erase(xuid);
    mLookupMap.erase(uuid.asString());

    return true;
}

SimulatedPlayer* FakePlayerManager::login(lfp::FakePlayer& fp) {
    if (!fp.mOnline && fp.login()) return fp.mPlayer;
    return nullptr;
}

bool FakePlayerManager::logout(lfp::FakePlayer& fakePlayer) { return fakePlayer.logout(true); }

lfp::FakePlayer* FakePlayerManager::tryGetFakePlayer(std::string const& key) const {
    auto iter = mLookupMap.find(key);
    if (iter != mLookupMap.end()) return iter->second;
    return {};
};

lfp::FakePlayer* FakePlayerManager::tryGetFakePlayer(mce::UUID const& uuid) const {
    if (!uuid) return {};
    auto fpIter = mFakePlayers.find(uuid);
    if (fpIter == mFakePlayers.end()) return {};
    return fpIter->second.get();
};

lfp::FakePlayer* FakePlayerManager::tryGetFakePlayer(Player const& player) const {
    auto uuid = player.getUuid();
    return tryGetFakePlayer(uuid);
};

lfp::FakePlayer* FakePlayerManager::tryGetFakePlayerByXuid(std::string const& xuid) const {
    auto iter = std::ranges::find_if(mFakePlayers, [&xuid](auto& pair) {
        return pair.second->getXuid() == xuid;
    });
    if (iter != std::ranges::end(mFakePlayers)) return iter->second.get();
    return {};
};

std::vector<lfp::FakePlayer*> FakePlayerManager::getTimeSortedFakePlayerList() {
    auto iter = mFakePlayers | std::views::values
              | std::views::transform([](auto& p) -> auto* { return p.get(); });
    std::vector<FakePlayer*> list(iter.begin(), iter.end());
    std::ranges::sort(list, [](auto left, auto right) {
        return left->mLastOnlineTime > right->mLastOnlineTime;
    });
    return list;
}


bool FakePlayerManager::setAutoLogin(FakePlayer& fp, bool autoLogin) {
    fp.mAutoLogin                    = autoLogin;
    auto                         res = mStorage->savePlayerInfo(fp.getUuid(), fp.serialize());
    event::FakePlayerUpdateEvent ev(fp);
    ll::event::EventBus::getInstance().publish(std::move(ev));
    return res;
}

bool lfp::FakePlayerManager::switchAutoLogin(FakePlayer& fp) {
    setAutoLogin(fp, !fp.shouldAutoLogin());
    return fp.shouldAutoLogin();
}

constexpr auto ContainerKeys = {"Armor", "EnderChestInventory", "Inventory", "Mainhand", "Offhand"};

bool FakePlayerManager::swapContainer(FakePlayer& fp, ServerPlayer& sp) {
    auto fpTag = fp.getPlayerData();
    if (!fpTag) return false;

    auto plTag = std::make_unique<CompoundTag>();
    if (!sp.save(*plTag)) return false;

    for (auto& key : ContainerKeys) {
        utils::nbt_utils::swapChildByKey(*plTag, *fpTag, key);
    }

    DefaultDataLoadHelper dlh;
    sp.load(*plTag, dlh);
    sp._sendDirtyActorData();
    sp.sendInventory(true);

    if (fp.isOnline()) {
        fp.mPlayer->load(*fpTag, dlh);
        fp.mPlayer->reload();
        fp.mPlayer->_sendDirtyActorData();
        fp.mPlayer->sendInventory(true);
    }
    return mStorage->savePlayerTag(fp.getUuid(), *fpTag);
}


inline void syncKey(CompoundTag& left, CompoundTag& right) {
    for (auto& [key, val] : left.mTags) {
        if (!right.contains(key)) {
            auto empty = Tag::newTag(val.getId());
            right.mTags.try_emplace(key, std::move(*empty));
        }
    }
    for (auto& [key, val] : right) {
        if (!left.contains(key)) {
            auto empty = Tag::newTag(val.getId());
            left.mTags.try_emplace(key, std::move(*empty));
        }
    }
}


bool FakePlayerManager::swapData(FakePlayer& fakePlayer, ServerPlayer& player) {
    auto plTag = std::make_unique<CompoundTag>();
    player.saveWithoutId(*plTag);
    auto fpTag = fakePlayer.getPlayerData();
    syncKey(*fpTag, *plTag);


    DefaultDataLoadHelper dlh;
    player.load(*fpTag, dlh);
    // player.getEntityContext().tryGetComponent<ActorUniqueIDComponent>()->mActorUniqueID->rawID =
    //     fpTag->at("UniqueID").get<Int64Tag>();

    DEBUGW("Player: before - after:\n{}", [&player, &fpTag]() {
        auto tag = std::make_unique<CompoundTag>();
        player.save(*tag);
        return debugCompareTag(*fpTag, *tag);
    }());
    // player.reload();

    player._sendDirtyActorData();
    player.sendInventory(true);

    if (fakePlayer.isOnline()) {
        // fakePlayer.mPlayer->remove();
        fakePlayer.mPlayer->load(*plTag, dlh);
        // fakePlayer.mPlayer->(plTag->getInt64("UniqueID"));
        DEBUGW("FakePlayer: before - after:\n{}", [&fakePlayer, &plTag]() {
            auto tag = std::make_unique<CompoundTag>();
            fakePlayer.mPlayer->save(*tag);
            return debugCompareTag(*plTag, *tag);
        }());
        fakePlayer.mPlayer->reload();
        fakePlayer.mPlayer->_sendDirtyActorData();
        fakePlayer.mPlayer->sendInventory(true);
    }
    return mStorage->savePlayerTag(fakePlayer.getUuid(), *plTag);
}


} // namespace lfp::inline manager
