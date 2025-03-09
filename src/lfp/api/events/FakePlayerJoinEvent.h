#pragma once

#include "lfp/api/events/base/FakePlayerEventBase.h"

namespace lfp::event {

class FakePlayerJoinEvent final : public FakePlayerEventBase {
public:
    constexpr explicit FakePlayerJoinEvent(FakePlayer& fp) : FakePlayerEventBase(fp) {}
    SimulatedPlayer& runtimePlayer() { return __super::runtimePlayer(); };
};

} // namespace lfp::event
