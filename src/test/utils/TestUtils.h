#pragma once
#include <functional>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/legacy/ActorUniqueID.h>
#include <test/utils/CoroUtils.h>

#include "mc/world/effect/EffectDuration.h"
#include "mc/world/effect/MobEffect.h"

#include "ll/api/io/Logger.h"

#include "lfp/manager/FakePlayer.h"
#include "lfp/manager/FakePlayerManager.h"


namespace lfp::test {

extern ll::io::Logger& testLogger;

bool executeCommandEx(
    std::string const&                          command,
    ::std::function<void(int, ::std::string&&)> output = [](int, ::std::string&&) {},
    DimensionType                               dim    = 0
);

int getTickingChunkCount(
    BlockSource const& region,
    BlockPos const&    bpos,
    int                range,
    bool               print = false
);

bool logIfNbtChange(CompoundTag const& oldTag, CompoundTag const& newTag);


template <typename Hook>
class HookHolder {
    static std::atomic<size_t> count;

public:
    HookHolder() {
        if (count.fetch_add(1) == 0) {
            Hook::hook();
        }
    }
    ~HookHolder() {
        if (count.fetch_sub(1) == 1) {
            // Probability causes deadlock?
            Hook::unhook();
        }
    }
};
template <typename Hook>
std::atomic<size_t> HookHolder<Hook>::count = 0ull;

struct FakePlayerHolder {
    FakePlayerManager& mManager;
    FakePlayer*        fp;
    FakePlayerHolder(std::string name) : mManager(lfp::FakePlayerManager::getManager()) {
        mManager.remove(name);
        fp = mManager.create(name);
    }
    ~FakePlayerHolder() { mManager.remove(*fp); }
    [[maybe_unused]] SimulatedPlayer& login() {
        fp->login();
        MobEffect::DAMAGE_RESISTANCE()
            ->applyEffects(*fp->getRuntimePlayer(), ::EffectDuration::INFINITE_DURATION(), 999);
        return *fp->getRuntimePlayer();
    }
    void                      logout() { fp->logout(); }
    void                      remove() { mManager.remove(*fp); }
    [[nodiscard]] FakePlayer* operator->() { return fp; }
    [[nodiscard]] FakePlayer& operator*() { return *fp; }
};


} // namespace lfp::test