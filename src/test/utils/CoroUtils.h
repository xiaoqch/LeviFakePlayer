#pragma once

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroPromise.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/Executor.h"

#include "mc/server/SimulatedPlayer.h"
#include "mc/world/level/BlockSource.h"


namespace lfp::test {

constexpr auto DEFAULT_TIMEOUT = ll::chrono::ticks{10 * 20};

[[nodiscard]] ll::coro::CoroTask<bool> waitUntil(
    std::function<bool()> checkCallback,
    ll::coro::Duration    timeout       = DEFAULT_TIMEOUT,
    ll::coro::Duration    checkInterval = ll::chrono::ticks{1}
);

ll::coro::CoroTask<bool>
waitForChunkLoaded(SimulatedPlayer& sp, ll::coro::Duration timeout = DEFAULT_TIMEOUT);

ll::coro::CoroTask<bool>
waitForRespawn(SimulatedPlayer& sp, ll::coro::Duration timeout = DEFAULT_TIMEOUT);

ll::coro::CoroTask<bool>
killAndWaitRespawn(SimulatedPlayer& sp, ll::coro::Duration timeout = DEFAULT_TIMEOUT);

ll::coro::CoroTask<bool>
teleportAndWaitPrepared(SimulatedPlayer& sp, Vec3 const& pos, DimensionType dimension);


} // namespace lfp::test