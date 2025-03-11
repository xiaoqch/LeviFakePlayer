#include "FixManager.h"
#include "lfp/Config.h"

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "ll/api/utils/ErrorUtils.h"

#include "lfp/LeviFakePlayer.h"

namespace lfp::fix {

LL_TYPE_STATIC_HOOK(
    FixAllSimulatedPlayerLoginHook,
    ll::memory::HookPriority::Normal,
    SimulatedPlayer,
    &SimulatedPlayer::create,
    ::SimulatedPlayer*,
    ::std::string const&                                  name,
    ::BlockPos const&                                     spawnPos,
    ::DimensionType                                       dimensionId,
    ::Bedrock::NotNullNonOwnerPtr<::ServerNetworkHandler> serverNetworkHandler,
    ::std::string const&                                  xuid
) {
    if (!lfp::FakePlayerManager::getManager().tryGetFakePlayerByXuid(xuid)) {
        lfp::LeviFakePlayer::getInstance().getFixManager().beforeFakePlayerLogin();
    }
    return origin(
        std::forward<::std::string const&>(name),
        std::forward<::BlockPos const&>(spawnPos),
        std::forward<::DimensionType>(dimensionId),
        std::forward<::Bedrock::NotNullNonOwnerPtr<::ServerNetworkHandler>>(serverNetworkHandler),
        std::forward<::std::string const&>(xuid)
    );
};


LL_TYPE_INSTANCE_HOOK(
    FixAllSimulatedPlayerLogoutHook,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::disconnect,
    void,
) {
    if (isSimulated() && !lfp::FakePlayerManager::getManager().tryGetFakePlayer(*this)) {
        ll::thread::ServerThreadExecutor::getDefault().executeAfter(
            []() { lfp::LeviFakePlayer::getInstance().getFixManager().afterFakePlayerLogout(); },
            ll::chrono::ticks{20}
        );
    }
    return origin();
};

void FixManager::setupSimulatedPlayerFix() {
    FixAllSimulatedPlayerLoginHook::hook();
    FixAllSimulatedPlayerLogoutHook::hook();
};
void FixManager::removeSimulatedPlayerFix() {
    FixAllSimulatedPlayerLoginHook::unhook();
    FixAllSimulatedPlayerLogoutHook::unhook();
};

bool FixManager::shouldFix(Player const* pl, bool forceFpOnly) {
    static bool fixAllSimulatedPlayer = lfp::getConfig().config.fix.fixAllSimulatedPlayer;
    if (nullptr == pl || !pl->isSimulated()) return false;
    // TODO: Test needed
    // Ignore SimulatedPlayer created by gametest
    if (nullptr != static_cast<SimulatedPlayer const*>(pl)->mGameTestHelper->get()) return false;
    if (fixAllSimulatedPlayer && !forceFpOnly) return true;
    return nullptr != lfp::FakePlayerManager::getManager().tryGetFakePlayer(*pl);
}

void FixManager::onPluginDisable() {
    afterFakePlayerLogout(true);
    removeSimulatedPlayerFix();
}
void FixManager::onPluginEnable() {
    if (mConfig.fixAllSimulatedPlayer) {
        setupSimulatedPlayerFix();
    }
}
void FixManager::afterFakePlayerLogout(bool pluginUnload) {
    if (!pluginUnload && !mConfig.dynamicUnloadFix) {
        return;
    }
    if (pluginUnload or count.fetch_sub(1) == 1) {
        try {
            deactivateStorageFix();
            deactivateFeatureFix();
            if (mConfig.fixAllSimulatedPlayer && pluginUnload) {
                removeSimulatedPlayerFix();
            }
        } catch (...) {
            lfp::LeviFakePlayer::getLogger().error("Error occurred when activating fixes");
            ll::error_utils::printCurrentException(lfp::LeviFakePlayer::getLogger());
        }
    }
}
void FixManager::beforeFakePlayerLogin() {
    if (count.fetch_add(1) == 0) {
        try {
            activateStorageFix();
            activateFeatureFix();
        } catch (...) {
            lfp::LeviFakePlayer::getLogger().error("Error occurred when activating fixes");
            ll::error_utils::printCurrentException(lfp::LeviFakePlayer::getLogger());
        }
    }
}
FakePlayer* FixManager::tryGetFakePlayer(Player const* pl) {
    return FakePlayerManager::getManager().tryGetFakePlayer(*pl);
}

} // namespace lfp::fix