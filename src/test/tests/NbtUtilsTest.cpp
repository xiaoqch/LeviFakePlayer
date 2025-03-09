#include "../TestManager.h"

#include "lfp/utils/NbtUtils.h"

namespace lfp::test {

LFP_TEST(NbtUtilsTest, SpawnByKeyBoth) {
    CompoundTag tag1{};
    CompoundTag tag2{};
    tag1["Armor"] = "Armor1";
    tag2["Armor"] = {"Armor2"};
    CompoundTag tag1c(tag1);
    CompoundTag tag2c(tag2);
    utils::nbt_utils::swapChildByKey(tag1, tag2, "Armor");
    EXPECT_TRUE(tag1.equals(tag2c));
    EXPECT_TRUE(tag2.equals(tag1c));
}

LFP_TEST(NbtUtilsTest, SpawnByKeyOne) {
    CompoundTag tag1{};
    CompoundTag tag2{};
    tag1["Armor"]  = "Armor1";
    tag1["Armor1"] = "Armor2";
    tag2["Armor1"] = "Armor3";
    CompoundTag tag1c(tag1);
    CompoundTag tag2c(tag2);
    utils::nbt_utils::swapChildByKey(tag1, tag2, "Armor");
    utils::nbt_utils::swapChildByKey(tag1, tag2, "Armor");
    EXPECT_TRUE(tag1.equals(tag1c));
    EXPECT_TRUE(tag2.equals(tag2c));
}

LFP_TEST(NbtUtilsTest, SpawnByKeyNone) {
    CompoundTag tag1{};
    CompoundTag tag2{};
    tag1["Armor1"] = "Armor1";
    tag1["Armor1"] = {"Armor1"};
    CompoundTag tag1c(tag1);
    CompoundTag tag2c(tag2);
    utils::nbt_utils::swapChildByKey(tag1, tag2, "Armor");
    EXPECT_TRUE(tag1.equals(tag1c));
    EXPECT_TRUE(tag2.equals(tag2c));
}

} // namespace lfp::test
