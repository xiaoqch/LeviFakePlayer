#pragma once

#include "lfp/api/events/base/FakePlayerEventBase.h"

namespace lfp::event {

class FakePlayerRemoveEvent final : public FakePlayerEventBase {
public:
    constexpr explicit FakePlayerRemoveEvent(FakePlayer& fp) : FakePlayerEventBase(fp) {}
    SimulatedPlayer& runtimePlayer() { return __super::runtimePlayer(); };
};

} // namespace lfp::event
