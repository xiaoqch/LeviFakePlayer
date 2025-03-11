#include "../TestManager.h"
#include "../utils/TestUtils.h"
#include <cassert>
#include <string>

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/shared_types/legacy/item/EquipmentSlot.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/storage/LevelData.h"


#include "ll/api/coro/Executor.h"
#include "ll/api/service/Bedrock.h"


#include "lfp/manager/FakePlayer.h"
#include "lfp/manager/FakePlayerManager.h"
#include "lfp/utils/NbtUtils.h"


using namespace ll::literals;


namespace lfp::test {


LFP_CO_TEST(FakePlayerTest, ChunkLoad) {
    FakePlayerHolder fp(mSetName);
    auto&            sp = fp.login();
    EXPECT_TRUE(co_await waitForChunkLoaded(sp));
    co_return;
}


LFP_CO_TEST(FakePlayerTest, Initialize) {
    FakePlayerHolder fp(mSetName);
    auto&            sp          = fp.login();
    auto             initialized = co_await waitUntil(
        [&]() {
            auto initialized  = sp.isPlayerInitialized();
            initialized      &= sp.mLocalPlayerInitialized;
            initialized      &= sp.mInitialized;
            initialized      &= sp.mIsInitialSpawnDone;
            initialized &= sp._isChunkSourceLoaded(sp.getPosition(), sp._getRegion());
            return initialized;
        },
        5 * 20_tick,
        5_tick
    );
    EXPECT_TRUE(initialized);
    testLogger.debug("chunkLimit: {}", sp._getSpawnChunkLimit());
    testLogger.debug("runtimeId: {}", sp.getRuntimeID().rawID);
    co_return;
}


LFP_CO_TEST(FakePlayerTest, Travel) {
    FakePlayerHolder fp(mSetName);
    auto&            sp = fp.login();
    EXPECT_TRUE(executeCommand(fmt::format("tp @p {} 66 0", sp.getPosition().x + 1200)));
    EXPECT_TRUE(co_await waitForChunkLoaded(sp));
    // executeCommand(fmt::format("execute as @p at @p run setblock ~ ~ ~ portal"));
    EXPECT_TRUE(co_await teleportAndWaitPrepared(sp, {0, 66, 0}, DimensionType(1)));

    EXPECT_TRUE(co_await teleportAndWaitPrepared(sp, {10, 66, 0}, DimensionType(2)));

    // check end credit
    executeCommand(fmt::format("/kill @e[type=ender_dragon]"));
    sp.teleport({1, 63, 0}, DimensionType(2));
    co_await 1_tick;
    EXPECT_TRUE(co_await waitUntil([&]() { return sp.getDimensionId().id == 0; }, 20 * 20_tick));
    EXPECT_TRUE(sp.isAlive());
    EXPECT_TRUE(co_await waitForChunkLoaded(sp));
    co_return;
}


LFP_CO_TEST(FakePlayerTest, Respawn) {
    FakePlayerHolder fp(mSetName);
    auto&            sp = fp.login();
    EXPECT_TRUE(executeCommand(fmt::format("kill @a")));
    co_await 20_tick;
    EXPECT_TRUE(co_await waitForChunkLoaded(sp));
    co_await 1_tick;
    EXPECT_TRUE(sp.isAlive());
};


LFP_CO_TEST(FakePlayerTest, OtherDimensionLogin) {
    FakePlayerHolder fp(mSetName);
    auto             sp = &fp.login();
    EXPECT_TRUE(co_await teleportAndWaitPrepared(*sp, {10, 66, 0}, DimensionType(2)));

    auto oldTag   = fp->getRuntimePlayerData();
    auto oldPos   = sp->getPosition();
    auto oldDimId = sp->getDimensionId();
    fp.logout();
    co_await 5_tick;
    sp          = &fp.login();
    auto newTag = fp->getRuntimePlayerData();
    oldTag->erase("ActiveEffects");
    newTag->erase("ActiveEffects");
    EXPECT_TRUE(!logIfNbtChange(*oldTag, *newTag));

    co_await 1_tick;
    auto newPos   = sp->getPosition();
    auto newDimId = sp->getDimensionId();
    EXPECT_TRUE(oldPos.distanceTo(newPos) < 4);
    EXPECT_TRUE(oldDimId == newDimId);
    EXPECT_TRUE(sp->getDimension().getDimensionId() == newDimId);
    EXPECT_TRUE(co_await waitForChunkLoaded(*sp));
};

LFP_CO_TEST(FakePlayerTest, RespawnPoint) {
    FakePlayerHolder fp(mSetName);
    EXPECT_TRUE(!fp->getRuntimePlayerData());
    auto& level      = ll::service::getLevel()->asServer();
    auto  worldSpawn = level.getLevelData().getSpawnPos();
    auto  sp         = &fp.login();
    EXPECT_TRUE(co_await waitForChunkLoaded(*sp));
    EXPECT_TRUE(sp->mPlayerRespawnPoint->mSpawnBlockPos->distanceTo(BlockPos::MIN()) < 1);
    EXPECT_TRUE(sp->mPlayerRespawnPoint->mPlayerPosition->distanceTo(Vec3::MIN()) < 1);
    // TODO init spawn
    // EXPECT_TRUE(co_await killAndWaitRespawn(*sp));
    auto initialSpawn = sp->getPosition();
    EXPECT_TRUE(initialSpawn.x - worldSpawn.x + initialSpawn.z - worldSpawn.z < 32);
    EXPECT_TRUE(initialSpawn.y < 400 && initialSpawn.y > 10);

    BlockPos testSpawn = {1000, 66, 0};
    EXPECT_TRUE(co_await teleportAndWaitPrepared(*sp, testSpawn, DimensionType(0)));
    auto nowPos = sp->getPosition();
    EXPECT_TRUE(nowPos.distanceTo(testSpawn) < 160);

    EXPECT_TRUE(co_await killAndWaitRespawn(*sp));
    nowPos = sp->getPosition();
    EXPECT_TRUE(nowPos.distanceTo(initialSpawn) < 16);

    // sp->setSpawnBlockRespawnPosition(testSpawn, 0);
    executeCommand(fmt::format("spawnpoint @a {} {} {}", testSpawn.x, testSpawn.y, testSpawn.z));
    EXPECT_TRUE(sp->mPlayerRespawnPoint->mPlayerPosition->distanceTo(testSpawn) < 1);
    EXPECT_TRUE(co_await killAndWaitRespawn(*sp));
    nowPos = sp->getPosition();
    EXPECT_TRUE(nowPos.distanceTo(testSpawn) < 160);
    EXPECT_TRUE(co_await teleportAndWaitPrepared(*sp, initialSpawn, DimensionType(0)));

    fp.logout();
    co_await 1_tick;
    sp = &fp.login();
    EXPECT_TRUE(co_await waitForChunkLoaded(*sp));
    EXPECT_TRUE(sp->mPlayerRespawnPoint->mPlayerPosition->distanceTo(testSpawn) < 1);
    nowPos = sp->getPosition();
    EXPECT_TRUE(nowPos.distanceTo(initialSpawn) < 2);

    EXPECT_TRUE(co_await killAndWaitRespawn(*sp));
    nowPos = sp->getPosition();
    EXPECT_TRUE(nowPos.distanceTo(testSpawn) < 160);
}

LFP_CO_TEST(FakePlayerTest, MultiLogin) {
    FakePlayerHolder fp1(mSetName + "1");
    FakePlayerHolder fp2(mSetName + "2");

    auto& sp1 = fp1.login();
    Vec3  tpPos1{1000, 66, 0};
    EXPECT_TRUE(co_await waitForChunkLoaded(sp1));
    EXPECT_TRUE(co_await teleportAndWaitPrepared(sp1, tpPos1, DimensionType(0)));
    EXPECT_TRUE(tpPos1.distanceTo(sp1.getPosition()) < 100);

    auto& sp2 = fp2.login();
    Vec3  tpPos2{0, 66, 1000};
    EXPECT_TRUE(sp1.getRuntimeID().rawID != sp2.getRuntimeID().rawID);
    EXPECT_TRUE(sp1.getClientSubId() != sp2.getClientSubId());
    EXPECT_TRUE(sp1.getOrCreateUniqueID() != sp2.getOrCreateUniqueID());

    EXPECT_TRUE(co_await waitForChunkLoaded(sp2));
    EXPECT_TRUE(co_await teleportAndWaitPrepared(sp2, tpPos2, DimensionType(0)));
    auto pos2 = sp2.getPosition();
    EXPECT_TRUE(pos2.distanceTo(tpPos2) < 100);

    fp2.logout();
    co_await 1_tick;
    EXPECT_TRUE(sp1.getLevel().getPlayer(sp1.getOrCreateUniqueID()));
    EXPECT_TRUE(fp1->getRuntimePlayerData());
};

inline std::string getHandItemName(SimulatedPlayer& sp) {
    auto name = sp.getItemSlot(::SharedTypes::Legacy::EquipmentSlot::HandSlot).getTypeName();
    // testLogger.debug("{}: {}", sp.getNameTag(), name);
    return name;
}

// fp1 simulate a normal player
LFP_CO_TEST(FakePlayerSwapTest, swapContainer) {
    FakePlayerHolder fp1(mSetName + "1");
    FakePlayerHolder fp2(mSetName + "2");
    auto&            sp1 = fp1.login();
    auto&            sp2 = fp2.login();
    EXPECT_TRUE(co_await waitForChunkLoaded(sp1));
    EXPECT_TRUE(executeCommand(fmt::format("give {} apple 10", fp1->getRealName())));
    EXPECT_TRUE(executeCommand(fmt::format("give {} stone 10", fp2->getRealName())));
    EXPECT_TRUE(getHandItemName(sp1) == "minecraft:apple");
    EXPECT_TRUE(getHandItemName(sp2) == "minecraft:stone");
    auto& manager = lfp::FakePlayerManager::getManager();
    manager.swapContainer(*fp2, sp1);
    EXPECT_TRUE(getHandItemName(sp1) == "minecraft:stone");
    EXPECT_TRUE(getHandItemName(sp2) == "minecraft:apple");
    manager.swapContainer(*fp2, sp1);
    EXPECT_TRUE(getHandItemName(sp1) == "minecraft:apple");
    EXPECT_TRUE(getHandItemName(sp2) == "minecraft:stone");
    co_return;
}


// fp1 login -> swap -> fp2 login swap -> check
// ->fp2 logout -> swap -> check -> swap -> check
LFP_CO_TEST(FakePlayerSwapTest, swapContainer2) {
    FakePlayerHolder fp1(mSetName + "1");
    FakePlayerHolder fp2(mSetName + "2");
    auto&            sp1 = fp1.login();
    fp2->login();
    co_await 1_tick;
    fp2->logout();
    co_await 1_tick;

    EXPECT_TRUE(co_await waitForChunkLoaded(sp1));
    EXPECT_TRUE(executeCommand(fmt::format("give {} apple 10", fp1->getRealName())));

    auto& manager = lfp::FakePlayerManager::getManager();
    auto  tag1    = fp2->getPlayerData();
    manager.swapContainer(*fp2, sp1);
    auto tag2 = fp2->getPlayerData();
    auto diff = lfp::utils::nbt_utils::tagDiff(*tag1, *tag2);
    EXPECT_TRUE(diff.first && diff.second && diff.first->contains("Mainhand"));
    EXPECT_TRUE(executeCommand(fmt::format("give {} stone 10", fp1->getRealName())));
    auto& sp2 = fp2.login();
    EXPECT_TRUE(co_await waitForChunkLoaded(sp2));
    auto tag3 = fp2->getRuntimePlayerData();
    EXPECT_TRUE(!logIfNbtChange(*tag2, *tag3));
    EXPECT_TRUE(getHandItemName(sp2) == "minecraft:apple");
    manager.swapContainer(*fp2, sp1);
    EXPECT_TRUE(getHandItemName(sp1) == "minecraft:apple");
    EXPECT_TRUE(getHandItemName(sp2) == "minecraft:stone");
    fp2->logout();
    co_await 1_tick;
    manager.swapContainer(*fp2, sp1);
    EXPECT_TRUE(getHandItemName(sp1) == "minecraft:stone");
    manager.swapContainer(*fp2, sp1);
    EXPECT_TRUE(getHandItemName(sp1) == "minecraft:apple");

    co_return;
}
} // namespace lfp::test