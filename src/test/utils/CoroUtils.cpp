#include "CoroUtils.h"
#include "../TestManager.h"

#include "mc/server/ServerPlayer.h"
#include "mc/world/level/BlockSource.h"

#include "ll/api/coro/CoroPromise.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/Executor.h"

using namespace ll::literals;

namespace lfp::test {

ll::coro::CoroTask<bool> waitUntil(
    std::function<bool()> checkCallback,
    ll::coro::Duration    timeout,
    ll::coro::Duration    checkInterval
) {
    auto deadline = ll::chrono::GameTickClock::now() + timeout;
    while (ll::chrono::GameTickClock::now() <= deadline) {
        if (checkCallback()) co_return true;
        co_await checkInterval;
    }
    co_return false;
}

ll::coro::CoroTask<bool> waitForChunkLoaded(SimulatedPlayer& sp, ll::coro::Duration timeout) {
    bool result     = true;
    auto chunkLimit = sp.::ServerPlayer::$_getSpawnChunkLimit();
    EXPECT_TRUE(chunkLimit > 0);
    auto&  region       = sp._getRegion();
    auto   pos          = sp.getFeetBlockPos();
    size_t tickingCount = 0;
    /// TODO: Should be base on simulation distance setting
    size_t neededTickingCount = 57; //
    int    range              = 6;  //

    result &= co_await waitUntil(
        [&]() {
            tickingCount = getTickingChunkCount(region, pos, range);
            return tickingCount >= neededTickingCount;
        },
        timeout,
        5_tick
    );
    EXPECT_TRUE(result);
    if (!result) getTickingChunkCount(region, pos, range, true);

    co_return result;
};

ll::coro::CoroTask<bool> waitForRespawn(SimulatedPlayer& sp, ll::coro::Duration timeout) {
    co_return co_await waitUntil([&]() { return sp.mRespawnReady; }, timeout, 1_tick);
};

ll::coro::CoroTask<bool> killAndWaitRespawn(SimulatedPlayer& sp, ll::coro::Duration timeout) {
    executeCommand("kill @a");
    /// TODO:
    co_await (3 * 20_tick);
    MobEffect::DAMAGE_RESISTANCE()->applyEffects(sp, ::EffectDuration::INFINITE_DURATION(), 999);
    co_return co_await waitForChunkLoaded(sp, timeout);
};

ll::coro::CoroTask<bool>
teleportAndWaitPrepared(SimulatedPlayer& sp, Vec3 const& pos, DimensionType dimension) {
    auto result = true;
    sp.teleport(pos, dimension);
    co_await 1_tick;
    EXPECT_TRUE(
        result &=
        co_await waitUntil([&]() { return sp.getDimensionId().id == dimension; }, 5 * 20_tick)
    );
    EXPECT_TRUE(result &= co_await waitForChunkLoaded(sp));
    co_return result;
}

} // namespace lfp::test