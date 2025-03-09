#pragma once

#include "lfp/api/events/base/FakePlayerEventBase.h"

namespace lfp::event {

class FakePlayerLeaveEvent final : public FakePlayerEventBase {
public:
    constexpr explicit FakePlayerLeaveEvent(FakePlayer& fp) : FakePlayerEventBase(fp) {}
    SimulatedPlayer& runtimePlayer() { return __super::runtimePlayer(); };
};

} // namespace lfp::event
