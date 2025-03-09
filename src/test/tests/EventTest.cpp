#include "../TestManager.h"
#include "ll/api/event/EventBus.h"

#include "lfp/api/events/FakePlayerCreatedEvent.h"
#include "lfp/api/events/FakePlayerJoinEvent.h"
#include "lfp/api/events/FakePlayerLeaveEvent.h"
#include "lfp/api/events/FakePlayerUpdateEvent.h"
#include "lfp/api/events/base/FakePlayerEventBase.h"
#include "lfp/manager/FakePlayer.h"

using namespace ll::literals;

namespace lfp::test {

constexpr auto name = "abc";

LFP_CO_TEST(EventTest, LeviEvent) {
    auto&  eventBus     = ll::event::EventBus::getInstance();
    size_t successCount = 0;

    auto& manager = lfp::FakePlayerManager::getManager();
    manager.remove(name);
    auto listener0 = eventBus.emplaceListener<event::FakePlayerCreatedEvent>([&](auto& ev) {
        if (ev.fakePlayer().getRealName() == name) {
            successCount += 1;
        }
    });
    auto listener1 = eventBus.emplaceListener<event::FakePlayerJoinEvent>([&](auto& ev) {
        if (ev.fakePlayer().getRealName() == name) {
            successCount += 1;
        }
    });
    auto listener2 = eventBus.emplaceListener<event::FakePlayerLeaveEvent>([&](auto& ev) {
        if (ev.fakePlayer().getRealName() == name) {
            successCount += 1;
        }
    });
    auto listener3 = eventBus.emplaceListener<event::FakePlayerLeaveEvent>([&](auto& ev) {
        if (ev.fakePlayer().getRealName() == name) {
            successCount += 1;
        }
    });
    auto listener4 = eventBus.emplaceListener<event::FakePlayerUpdateEvent>([&](auto& ev) {
        if (ev.fakePlayer().getRealName() == name) {
            successCount += 1;
        }
    });
    co_await 0_tick;
    auto fp = manager.create(name);
    manager.login(*fp);
    manager.setAutoLogin(*fp, true);
    co_await 1_tick;
    manager.logout(*fp);
    manager.remove(*fp);
    co_await 0_tick;

    eventBus.removeListener(listener0);
    eventBus.removeListener(listener1);
    eventBus.removeListener(listener2);
    eventBus.removeListener(listener3);
    EXPECT_TRUE(successCount == 5);
    co_return;
}

} // namespace lfp::test