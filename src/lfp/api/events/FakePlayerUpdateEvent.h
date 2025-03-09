#pragma once

#include "lfp/api/events/base/FakePlayerEventBase.h"

namespace lfp::event {

class FakePlayerUpdateEvent final : public FakePlayerEventBase {
public:
    constexpr explicit FakePlayerUpdateEvent(FakePlayer& fp) : FakePlayerEventBase(fp) {}
};

} // namespace lfp::event
