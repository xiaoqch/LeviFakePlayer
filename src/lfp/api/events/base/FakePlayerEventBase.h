#pragma once

#include "lfp/api/base/Global.h"

#include "mc/nbt/CompoundTag.h"
#include "mc/server/SimulatedPlayer.h"

#include "ll/api/event/Event.h"

#include "lfp/manager/FakePlayer.h"

namespace lfp::inline manager {

class FakePlayer;
class FakePlayerManager;

} // namespace lfp::inline manager

namespace lfp::event::inline base {

class FakePlayerEventBase : public ll::event::Event {
protected:
    constexpr explicit FakePlayerEventBase(FakePlayer& fp) : mFakePlayer(fp) {}

    SimulatedPlayer& runtimePlayer() const { return *mFakePlayer.getRuntimePlayer(); };

public:
    FakePlayer& mFakePlayer;

    std::string name() const { return mFakePlayer.getRealName(); }
    std::string xuid() const { return mFakePlayer.getXuid(); }
    bool        isOnline() const { return mFakePlayer.isOnline(); }

    FakePlayer& fakePlayer() const { return mFakePlayer; };

    LFPAPI std::unique_ptr<CompoundTag> playerData() const;
    LFPAPI void                         serialize(CompoundTag& tag) const override;
};

} // namespace lfp::event::inline base
