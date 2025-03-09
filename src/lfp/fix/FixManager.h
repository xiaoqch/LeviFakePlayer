#pragma once

#include <atomic>

#include "mc/world/actor/player/Player.h"

#include "lfp/Config.h"

namespace lfp::inline manager {
class FakePlayer;
}

namespace lfp::fix {


class FixManager {
    std::atomic_uint                       count;
    LeviFakePlayerConfig::FixConfig const& mConfig;

    void setupSimulatedPlayerFix();
    void removeSimulatedPlayerFix();
    void activateStorageFix();
    void activateFeatureFix();
    void deactivateStorageFix();
    void deactivateFeatureFix();


public:
    FixManager(LeviFakePlayerConfig const& config) : mConfig(config.config.fix) {}

    [[nodiscard]] static bool shouldFix(Player const* pl, bool forceFpOnly = false);
    [[nodiscard]] static bool shouldFix(Player const& pl, bool forceFpOnly = false) {
        return shouldFix(&pl, forceFpOnly);
    }

    [[nodiscard]] static FakePlayer* tryGetFakePlayer(Player const* pl);

    // use event system?
    void beforeFakePlayerLogin();
    void afterFakePlayerLogout(bool pluginUnload = false);

    void onPluginEnable();
    void onPluginDisable();
};

// CoreFix
struct PlayerIdsFix;
struct SavePlayerDataFix;
struct LoadPlayerDataFix;


} // namespace lfp::fix