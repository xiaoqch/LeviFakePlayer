#pragma once

#include <cassert>
#include <concepts>
#include <coroutine>
#include <filesystem>
#include <functional>
#include <memory>
#include <ranges>
#include <string>

#include "mc/nbt/CompoundTag.h"
#include "mc/platform/UUID.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"

#include "ll/api/base/Concepts.h"
#include "ll/api/coro/Generator.h"
#include "ll/api/io/Logger.h"

#include "lfp/fix/FixManager.h"
#include "lfp/manager/FakePlayer.h"

namespace lfp {

class LeviFakePlayer;
namespace fix {
struct PlayerIdsFix;
}
} // namespace lfp

namespace lfp::inline manager {

class FakePlayer;
class FakePlayerManager;
class FakePlayerStorage;

template <typename T>
concept ValidFakePlayerIndex =
    ll::concepts::IsString<T> || std::derived_from<T, mce::UUID> || std::derived_from<T, Player>;

class FakePlayerManager {
private:
    std::unique_ptr<FakePlayerStorage> mStorage;
    ll::io::Logger&                    mLogger;

    std::unordered_map<mce::UUID, std::unique_ptr<FakePlayer>> mFakePlayers;
    std::unordered_map<std::string, FakePlayer*>               mLookupMap;
    // std::vector<std::string>                                   smSortedNames;

private:
    void initFakePlayers();


public:
    FakePlayerManager(std::filesystem::path const& dbPath);
    ~FakePlayerManager();
    FakePlayerManager(const FakePlayerManager&)            = delete;
    FakePlayerManager& operator=(const FakePlayerManager&) = delete;
    FakePlayerManager(FakePlayerManager&&)                 = delete;
    FakePlayerManager& operator=(FakePlayerManager&&)      = delete;

    int                                        savePlayers(bool onlineOnly = false);
    [[nodiscard]] std::unique_ptr<CompoundTag> loadPlayerData(FakePlayer const& fp);
    // std::unique_ptr<CompoundTag> loadPlayerInfo(FakePlayer const& fp);
    bool saveFakePlayerInfo(FakePlayer& fp);
    bool saveRuntimeData(FakePlayer& fp);
    bool importData_JavaFakePlayer(std::string const& name);


    bool savePlayer(FakePlayer& fp);

    template <ValidFakePlayerIndex T>
    inline bool saveRuntimeData(T& id) {
        auto fp = tryGetFakePlayer(id);
        if (fp) return saveRuntimeData(*fp);
        return false;
    }

    template <ValidFakePlayerIndex T>
    inline bool savePlayer(T& id) {
        auto fp = tryGetFakePlayer(id);
        if (fp) return savePlayer(*fp);
        return false;
    }

    template <ValidFakePlayerIndex T>
    [[nodiscard]] inline auto loadPlayerData(T const& id) {
        auto fp = tryGetFakePlayer(id);
        if (fp) return loadPlayerData(*fp);
        return false;
    }
    friend class lfp::LeviFakePlayer;
    friend class lfp::FakePlayer;
    // hooks
    friend struct lfp::fix::PlayerIdsFix;
    friend struct lfp::fix::SavePlayerDataFix;
    friend struct lfp::fix::LoadPlayerDataFix;

public:
    [[nodiscard]] static FakePlayerManager& getManager();

    [[nodiscard]] ll::coro::Generator<FakePlayer&> iter() {
        for (auto& fp : mFakePlayers | std::views::values) {
            co_yield *fp;
        }
    };
    inline void forEachFakePlayer(std::function<void(FakePlayer const& fp)> callback) const {
        for (auto& fp : mFakePlayers | std::views::values) {
            callback(*fp);
        }
    }
    [[nodiscard]] std::vector<FakePlayer*> getTimeSortedFakePlayerList();

    FakePlayer*
    create(std::string name, bool login = false, std::unique_ptr<CompoundTag> playerData = {});
    FakePlayer*      create(std::string name, BlockPos const& spawnPos, DimensionType dimension);
    bool             remove(FakePlayer& fp);
    SimulatedPlayer* login(FakePlayer& fp);
    bool             logout(FakePlayer& fp);

    template <ValidFakePlayerIndex T>
    inline SimulatedPlayer* login(T const& id) {
        FakePlayer* fp = tryGetFakePlayer(id);
        if (fp) return login(*fp);
        return nullptr;
    }
    template <ValidFakePlayerIndex T>
    inline bool logout(T const& id) {
        auto fp = tryGetFakePlayer(id);
        if (fp) return logout(*fp);
        return false;
    }

    template <typename T>
    inline bool remove(T const& id) {
        auto fp = tryGetFakePlayer(id);
        if (fp) return remove(*fp);
        return false;
    }

    [[nodiscard]] FakePlayer* tryGetFakePlayer(Player const& player) const;
    [[nodiscard]] FakePlayer* tryGetFakePlayer(mce::UUID const& uuid) const;
    [[nodiscard]] FakePlayer* tryGetFakePlayer(std::string const& nameOrId) const;

    FakePlayer* tryGetFakePlayerByXuid(std::string const& xuid) const;

    bool swapContainer(FakePlayer& fp, ServerPlayer& sp);

    bool setAutoLogin(FakePlayer& fp, bool autoLogin);
    bool switchAutoLogin(FakePlayer& fp);

private:
    bool swapData(FakePlayer& fp, ServerPlayer& sp);
};

} // namespace lfp::inline manager
