#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include "ll/api/io/Logger.h"
#include "ll/api/mod/NativeMod.h"

#include "lfp/Config.h"
#include "lfp/fix/FixManager.h"
#include "lfp/manager/FakePlayerManager.h"

namespace lfp {

class LeviFakePlayer {

public:
    [[nodiscard]] static LeviFakePlayer&        getInstance();
    [[nodiscard]] static std::filesystem::path  getDataPath(std::string_view fileName);
    [[nodiscard]] inline static ll::io::Logger& getLogger() {
        return getInstance().getSelf().getLogger();
    }
    [[nodiscard]] ll::mod::NativeMod&                getSelf() const { return mSelf; }
    [[nodiscard]] inline FakePlayerManager&          getManager() { return *mManager; }
    [[nodiscard]] inline fix::FixManager&            getFixManager() { return *mFixManager; };
    [[nodiscard]] inline LeviFakePlayerConfig const& getConfig() { return lfp::getConfig(); }

    LeviFakePlayer();


    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    // TODO: Implement this method if you need to unload the mod.
    // /// @return True if the mod is unloaded successfully.
    // bool unload();

private:
    ll::mod::NativeMod&                mSelf;
    std::unique_ptr<fix::FixManager>   mFixManager;
    std::unique_ptr<FakePlayerManager> mManager;
};

} // namespace lfp
