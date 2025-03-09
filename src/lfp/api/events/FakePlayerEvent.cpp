#include "lfp/api/events/FakePlayerCreatedEvent.h"
#include "lfp/api/events/FakePlayerJoinEvent.h"
#include "lfp/api/events/FakePlayerLeaveEvent.h"
#include "lfp/api/events/FakePlayerRemoveEvent.h"
#include "lfp/api/events/FakePlayerUpdateEvent.h"

#include "ll/api/event/Emitter.h"
#include "ll/api/event/EmitterBase.h"

namespace lfp::event::inline base {

void FakePlayerEventBase::serialize(CompoundTag& tag) const {
    __super::serialize(tag);
    mFakePlayer.serialize(tag);
    tag["xuid"] = mFakePlayer.getXuid();
};

std::unique_ptr<CompoundTag> FakePlayerEventBase::playerData() const {
    if (mFakePlayer.isOnline()) {
        return mFakePlayer.getRuntimePlayerData();
    }
    return mFakePlayer.getSavedPlayerData();
}


} // namespace lfp::event::inline base

namespace lfp::event {

template <typename Ev>
static std::unique_ptr<ll::event::EmitterBase> emitterFactory();

template <typename Ev>
class FakePlayerEventEmitterBase : public ll::event::Emitter<emitterFactory<Ev>, Ev> {};

template <typename Ev>
static std::unique_ptr<ll::event::EmitterBase> emitterFactory() {
    return std::make_unique<FakePlayerEventEmitterBase<Ev>>();
}

class FakePlayerCreatedEventEmitter : public FakePlayerEventEmitterBase<FakePlayerCreatedEvent> {};
class FakePlayerJoinEventEmitter : public FakePlayerEventEmitterBase<FakePlayerJoinEvent> {};
class FakePlayerUpdateEventEmitter : public FakePlayerEventEmitterBase<FakePlayerUpdateEvent> {};
class FakePlayerLeaveEventEmitter : public FakePlayerEventEmitterBase<FakePlayerLeaveEvent> {};
class FakePlayerRemoveEventEmitter : public FakePlayerEventEmitterBase<FakePlayerRemoveEvent> {};

} // namespace lfp::event