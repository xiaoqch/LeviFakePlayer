#include "ll/api/Config.h"

#include "lfp/Config.h"
#include "lfp/LeviFakePlayer.h"

namespace lfp {

[[nodiscard]] LeviFakePlayerConfig& getConfig() {
    static LeviFakePlayerConfig config = []() {
        LeviFakePlayerConfig tmp;
        auto path = lfp::LeviFakePlayer::getInstance().getSelf().getConfigDir() / "Config.json";
        if (!ll::config::loadConfig(tmp, path)) {
            ll::config::saveConfig(tmp, path);
            lfp::LeviFakePlayer::getLogger().info("Config file has updated");
        }
        return tmp;
    }();
    return config;
};

bool saveConfig() {
    auto path = lfp::LeviFakePlayer::getInstance().getSelf().getConfigDir() / "Config.json";
    if (!ll::config::saveConfig(getConfig(), path)) {
        lfp::LeviFakePlayer::getLogger().error("Error occurred when load config");
        return false;
    };
    return true;
};

} // namespace lfp