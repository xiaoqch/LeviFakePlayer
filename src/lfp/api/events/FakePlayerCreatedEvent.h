#pragma once

#include "lfp/api/events/base/FakePlayerEventBase.h"

namespace lfp::event {

class FakePlayerCreatedEvent final : public FakePlayerEventBase {
public:
    constexpr explicit FakePlayerCreatedEvent(FakePlayer& fp) : FakePlayerEventBase(fp) {}
};

} // namespace lfp::event
