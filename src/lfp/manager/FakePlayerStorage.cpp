#include "FakePlayerStorage.h"

#include <coroutine>
#include <ranges>
#include <string_view>

#include "lfp/LeviFakePlayer.h"
#include "ll/api/utils/StringUtils.h"

namespace lfp::inline manager {

FakePlayerStorage::FakePlayerStorage(std::filesystem::path const& storagePath)
: mLock(),
  mLogger(lfp::LeviFakePlayer::getLogger()),
  mStorage(std::make_unique<ll::data::KeyValueDB>(storagePath)) {
    auto version = mStorage->get("VERSION").value_or("0");
    mVersion     = ll::string_utils::svtoi(version).value_or(0);
    if (mVersion != sCurrentVersion) upgrade(mVersion);
}

bool FakePlayerStorage::removePlayerData(mce::UUID uuid) {
    std::lock_guard<StorageLock> lock(mLock);

    auto storageId = getStorageId(uuid);
    auto serverId  = getServerId(uuid);

    auto res = mStorage->del(storageId);
    if (!res) {
        mLogger.error("Error in {} - {}", __FUNCTION__, storageId);
        return false;
    }
    res = mStorage->del(serverId);
    if (!res) {
        mLogger.error("Error in {} - {}", __FUNCTION__, serverId);
        return false;
    }
    return true;
};

bool FakePlayerStorage::savePlayerTag(mce::UUID uuid, CompoundTag const& tag) {
    std::lock_guard<StorageLock> lock(mLock);
    auto                         serverId = getServerId(uuid);
    if (mStorage->set(serverId, tag.toBinaryNbt(true))) return true;
    mLogger.error("Error in {} - {}", __FUNCTION__, serverId);
    return false;
};

bool FakePlayerStorage::savePlayerInfo(mce::UUID uuid, CompoundTag const& tag) {
    std::lock_guard<StorageLock> lock(mLock);
    auto                         storageId = getStorageId(uuid);
    auto                         info      = tag.toBinaryNbt(true);
    if (mStorage->set(storageId, info)) return true;
    mLogger.error("Error in {} - {}", __FUNCTION__, storageId);
    return false;
};

std::optional<std::string> FakePlayerStorage::loadPlayerData(mce::UUID uuid) {
    std::lock_guard<StorageLock> lock(mLock);
    auto                         serverId = getServerId(uuid);
    return mStorage->get(serverId);
};

std::unique_ptr<CompoundTag> FakePlayerStorage::loadPlayerTag(mce::UUID uuid) {
    auto data = loadPlayerData(uuid);
    if (data && !data->empty())
        return std::make_unique<CompoundTag>(*CompoundTag::fromBinaryNbt(*data, true));
    return {};
};

std::unique_ptr<CompoundTag> FakePlayerStorage::loadPlayerInfo(mce::UUID uuid) {
    std::lock_guard<StorageLock> lock(mLock);
    auto                         storageId = getStorageId(uuid);
    auto                         data      = mStorage->get(storageId);
    if (data && !data->empty())
        return std::make_unique<CompoundTag>(*CompoundTag::fromBinaryNbt(*data, true));
    return {};
};

ll::coro::Generator<std::pair<mce::UUID, std::unique_ptr<CompoundTag>>> FakePlayerStorage::iter() {
    for (auto [key, val] : mStorage->iter()) {
        if (key.starts_with("player_") && !key.starts_with("player_server_")) {
            auto tag = std::make_unique<CompoundTag>(*CompoundTag::fromBinaryNbt(val, true));
            co_yield {mce::UUID::fromString(std::string(key.substr(7))), std::move(tag)};
        }
    }
}
} // namespace lfp::inline manager