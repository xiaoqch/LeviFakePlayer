#pragma once

#include <cassert>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "ll/api/service/Bedrock.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/platform/UUID.h"

#include "ll/api/coro/Generator.h"
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/io/Logger.h"

#include "lfp/manager/FakePlayerManager.h"


namespace lfp::inline manager {

typedef std::mutex StorageLock;
class FakePlayerManager;

class FakePlayerStorage {
protected:
    StorageLock                           mLock;
    ll::io::Logger&                       mLogger;
    std::unique_ptr<ll::data::KeyValueDB> mStorage;
    int                                   mVersion;

    constexpr static int sCurrentVersion = 1;

public:
    FakePlayerStorage(std::filesystem::path const& storagePath);
    friend class FakePlayerManager;
    inline static std::string getStorageId(mce::UUID uuid) { return "player_" + uuid.asString(); }
    inline static std::string getServerId(mce::UUID uuid) {
        return "player_server_" + uuid.asString();
    }


protected:
    // inline bool fixDatabase() { return false; };
    inline std::vector<mce::UUID> allPlayerId() {
        std::vector<mce::UUID> uuids;
        for (auto [key, data] : mStorage->iter()) {
            if (!key.starts_with("player_")) continue;
            if (key.starts_with("player_server_")) continue;
            uuids.push_back(mce::UUID::fromString(std::string(key.substr(sizeof("player_") - 1))));
        }
        return uuids;
    }
    inline void upgrade(int version) {
        if (version < 1) {
            for (auto& uuid : allPlayerId()) {
                auto info = loadPlayerInfo(uuid);
                auto data = loadPlayerTag(uuid);
                if (data) {
                    (*info)["uniqueId"] = (*data)["UniqueID"];
                } else {
                    (*info)["uniqueId"] = ll::service::getLevel()->getNewUniqueID().rawID;
                }
                (*info)["uuid"] = uuid.asString();
                (void)savePlayerInfo(uuid, *info);
            }
        }
        mVersion               = sCurrentVersion;
        std::string newVersion = std::to_string(mVersion);
        mStorage->set("VERSION", newVersion);
    }

public:
    [[nodiscard]] ll::coro::Generator<std::pair<mce::UUID, std::unique_ptr<CompoundTag>>> iter();
    [[nodiscard]] bool removePlayerData(mce::UUID uuid);
    [[nodiscard]] bool savePlayerTag(mce::UUID uuid, CompoundTag const& tag);
    [[nodiscard]] bool savePlayerInfo(mce::UUID uuid, CompoundTag const& tag);
    [[nodiscard]] bool savePlayerTag(mce::UUID uuid, std::unique_ptr<CompoundTag> tag) {
        return savePlayerTag(uuid, *tag);
    }
    [[nodiscard]] bool savePlayerInfo(mce::UUID uuid, std::unique_ptr<CompoundTag> tag) {
        return savePlayerInfo(uuid, *tag);
    }


    [[nodiscard]] std::optional<std::string>   loadPlayerData(mce::UUID uuid);
    [[nodiscard]] std::unique_ptr<CompoundTag> loadPlayerTag(mce::UUID uuid);
    [[nodiscard]] std::unique_ptr<CompoundTag> loadPlayerInfo(mce::UUID uuid);
};
} // namespace lfp::inline manager