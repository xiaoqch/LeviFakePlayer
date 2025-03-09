
#include "../TestManager.h"
#include "../utils/TestUtils.h"

#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components/ActorUniqueIDComponent.h"
#include "mc/gametest/MinecraftGameTestHelper.h"
#include "mc/gametest/NullGameTestHelper.h"
#include "mc/gametest/framework/BaseGameTestHelper.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/Int64Tag.h"
#include "mc/platform/UUID.h"
#include "mc/scripting/modules/gametest/ScriptSimulatedPlayer.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/ServerPlayer.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/Minecraft.h"
#include "mc/world/level/GameplayUserManager.h"
#include "mc/world/level/storage/DBStorage.h"

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"

#include "lfp/manager/FakePlayerManager.h"
#include "lfp/manager/FakePlayerStorage.h"
#include "lfp/utils/SimulatedPlayerUtils.h"


using namespace ll::literals;

namespace lfp::test {

namespace {
int            SuccessCount       = 0;
constexpr auto TestFakePlayerName = "LFP_Test";
}; // namespace

LL_TYPE_INSTANCE_HOOK(
    Test_LoadPlayerDataFix_Before,
    ll::memory::HookPriority::Lowest,
    LevelStorage,
    &LevelStorage::loadServerPlayerData,
    ::std::unique_ptr<::CompoundTag>,
    ::Player const& client,
    bool            isXboxLive
) {
    EXPECT_TRUE(lfp::utils::sp_utils::isFakePlayer(client));
    auto loadedTag = origin(
        std::forward<decltype(client)>(client),
        std::forward<decltype(isXboxLive)>(isXboxLive)
    );
    testLogger.debug("LevelStorage::loadServerPlayerData({}, {})", client.getNameTag(), isXboxLive);
    if (loadedTag) {
        testLogger.error("Storage data for SimulatedPlayer is not empty");
    } else {
        SuccessCount++;
    }
    return std::forward<decltype(loadedTag)>(loadedTag);
}

LL_TYPE_INSTANCE_HOOK(
    Test_LoadPlayerDataFix_After,
    ll::memory::HookPriority::Highest,
    LevelStorage,
    &LevelStorage::loadServerPlayerData,
    ::std::unique_ptr<::CompoundTag>,
    ::Player const& client,
    bool            isXboxLive
) {
    EXPECT_TRUE(lfp::utils::sp_utils::isFakePlayer(client));
    auto& manager  = lfp::FakePlayerManager::getManager();
    auto  savedTag = manager.tryGetFakePlayer(client.getXuid())->getSavedPlayerData();

    auto loadedTag = origin(
        std::forward<decltype(client)>(client),
        std::forward<decltype(isXboxLive)>(isXboxLive)
    );

    if (savedTag && !savedTag->equals(*loadedTag)) {
        testLogger.error("SimulatedPlayer data");
    } else {
        SuccessCount++;
    }
    return std::forward<decltype(loadedTag)>(loadedTag);
}

LFP_CO_TEST(ManagerTest, Create) {
    auto& manager = lfp::FakePlayerManager::getManager();
    manager.remove(TestFakePlayerName);

    auto fp = manager.create(TestFakePlayerName);
    EXPECT_TRUE(fp);
    // EXPECT_TRUE(manager.mStorage->loadPlayerInfo(fp->getUuid()));
    EXPECT_TRUE(!manager.loadPlayerData(*fp));
    co_return;
}

LFP_CO_TEST(ManagerTest, Login) {
    HookHolder<Test_LoadPlayerDataFix_Before> before;
    HookHolder<Test_LoadPlayerDataFix_After>  after;

    auto& manager = lfp::FakePlayerManager::getManager();
    auto& level   = ll::service::getLevel()->asServer();
    auto  fp      = manager.tryGetFakePlayer(TestFakePlayerName);
    EXPECT_TRUE(fp->login());
    co_await 1_tick;
    EXPECT_TRUE(fp->getRuntimePlayer() == level.getPlayer(fp->getUuid()));

    auto prevTag = fp->getRuntimePlayerData();

    EXPECT_TRUE(fp->logout());
    co_await 1_tick;
    EXPECT_TRUE(!ll::service::getLevel()->getPlayer(fp->getUuid()));


    auto savedTag = fp->getSavedPlayerData();
    EXPECT_TRUE(prevTag->equals(*savedTag));

    auto const uidInTag = (::ActorUniqueID)(*savedTag)["UniqueID"].get<Int64Tag>().data;

    fp->login();
    co_await 1_tick;

    auto sp = fp->getRuntimePlayer();

    auto const uidComp = sp->getEntityContext().tryGetComponent<ActorUniqueIDComponent>();
    EXPECT_TRUE(uidComp);
    auto const uidDirectly = sp->getOrCreateUniqueID();
    testLogger
        .debug("uids: {},{},{}", uidInTag.rawID, uidComp->mActorUniqueID->rawID, uidDirectly.rawID);
    EXPECT_TRUE(uidInTag == uidDirectly);
    EXPECT_TRUE(uidInTag == uidComp->mActorUniqueID);

    EXPECT_TRUE(fp->getXuid() == sp->getXuid());
    EXPECT_TRUE(ll::service::getLevel()->getPlayer(uidInTag) == sp);
    EXPECT_TRUE(ll::service::getLevel()->getPlayer(fp->getRealName()) == sp);
    EXPECT_TRUE(ll::service::getLevel()->getPlayer(fp->getUuid()) == sp);
    // Failed
    // EXPECT_TRUE(ll::service::getLevel()->getPlayerByXuid(fp->getXuid()) == sp);
    EXPECT_TRUE(ll::service::getLevel()->getRuntimePlayer(sp->getRuntimeID()) == sp);

    fp->logout();
    co_await 1_tick;
    EXPECT_TRUE(!ll::service::getLevel()->getPlayer(fp->getUuid()));
    EXPECT_TRUE(SuccessCount == 4);
    co_return;
}

LFP_CO_TEST(ManagerTest, Remove) {
    auto& manager = lfp::FakePlayerManager::getManager();
    auto  fp      = manager.tryGetFakePlayer(TestFakePlayerName);
    auto  uuid    = fp->getUuid();
    auto  xuid    = fp->getXuid();
    auto  name    = fp->getRealName();
    manager.remove(*fp);
    co_await 0_tick;

    EXPECT_TRUE(!manager.tryGetFakePlayer(uuid));
    EXPECT_TRUE(!manager.tryGetFakePlayer(name));
    EXPECT_TRUE(!manager.tryGetFakePlayer(xuid));
    // EXPECT_TRUE(!manager.mStorage->loadPlayerInfo(uuid.asString()));
    // EXPECT_TRUE(!manager.mStorage->loadPlayerTag(uuid.asString()));
    co_return;
}

} // namespace lfp::test
