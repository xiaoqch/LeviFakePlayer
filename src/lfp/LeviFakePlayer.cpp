#include <memory>

#include "lfp/Config.h"
#include "lfp/LeviFakePlayer.h"

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/io/LogLevel.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/thread/ServerThreadExecutor.h"

#include "lfp/api/rcapi//rcapi.h"
#include "lfp/command/FakePlayerCommand.h"
#include "lfp/command/TickingCommand.h"
#include "lfp/fix/FixManager.h"
#include "lfp/manager/FakePlayerManager.h"

namespace lfp {

constexpr auto STORAGE_PATH = "storage";

LeviFakePlayer::LeviFakePlayer() : mSelf(*ll::mod::NativeMod::current()) {}

std::filesystem::path LeviFakePlayer::getDataPath(std::string_view fileName) {
    auto path = getInstance().getSelf().getDataDir() / fileName;
    if (!std::filesystem::exists(path.parent_path())) {
        std::filesystem::create_directories(path.parent_path());
    }
    return path;
}

LeviFakePlayer& LeviFakePlayer::getInstance() {
    static LeviFakePlayer instance;
    return instance;
}

bool LeviFakePlayer::load() {
    auto& logger = getSelf().getLogger();
    auto& config = getConfig();
    if (auto res = ll::i18n::getInstance().load(getSelf().getLangDir()); !res) {
        getLogger().error("i18n load failed");
        res.error().log(getLogger());
    }
    logger.setLevel(config.config.logLevel);
#ifdef LFP_DEBUG
    logger.setLevel(ll::io::LogLevel::Trace);
#endif
    logger.debug("Loading...");

    mFixManager = std::make_unique<fix::FixManager>(config);
    auto path   = lfp::LeviFakePlayer::getDataPath(STORAGE_PATH);
    mManager    = std::make_unique<FakePlayerManager>(path);
    if (!lfp::api::ExportRemoteCallApis()) {
        logger.error("Failed to export api for LegacyScriptEngine!");
    }
    return true;
}

bool LeviFakePlayer::enable() {
    getSelf().getLogger().debug("Enabling...");

    mFixManager->onPluginEnable();

    auto& config = lfp::LeviFakePlayer::getInstance().getConfig().config;
    if (config.command.enabled) {
        FakePlayerCommand::setup(config.command);
#ifndef LFP_DEBUG
        if (config.command.enableTickingCommand && *config.command.enableTickingCommand)
#endif // always enabled in debug build
            TickingCommand::setup();
    }
    // TODO: crash
    // auto& eventBus = ll::event::EventBus::getInstance();
    // auto stoppingListener = eventBus.emplaceListener<ll::event::ServerStoppingEvent>([](auto) {
    //     lfp::FakePlayerManager::getManager().savePlayers();
    // });
    ll::thread::ServerThreadExecutor::getDefault().executeAfter(
        []() {
            for (auto& fp : lfp::FakePlayerManager::getManager().iter()) {
                if (fp.shouldAutoLogin()) fp.login();
            }
        },
        ll::chrono::ticks{5 * 20}
    );
    return true;
}

bool LeviFakePlayer::disable() {
    getSelf().getLogger().debug("Disabling...");

    mFixManager->onPluginDisable();
    return true;
}

LL_REGISTER_MOD(lfp::LeviFakePlayer, lfp::LeviFakePlayer::getInstance());

} // namespace lfp
