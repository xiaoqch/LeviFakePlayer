#pragma once

#include <memory>
#include <utility>

#include "mc/deps/core/math/Vec3.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/FloatTag.h"
#include "mc/nbt/ListTag.h"
#include "mc/world/level/BlockPos.h"

namespace lfp::utils::nbt_utils {


bool swapChildByKey(CompoundTag& comp1, CompoundTag& comp2, std::string_view key);

// TODO: Test needed
bool tagDiff(
    CompoundTag const& left,
    CompoundTag const& right,
    CompoundTag&       leftResult,
    CompoundTag&       rightResult,
    size_t             recursionLimit = std::numeric_limits<size_t>::max()
);


bool tagDiff(
    ListTag const& left,
    ListTag const& right,
    ListTag&       leftResult,
    ListTag&       rightResult,
    size_t         recursionLimit = std::numeric_limits<size_t>::max()
);

std::pair<std::unique_ptr<ListTag>, std::unique_ptr<ListTag>> tagDiff(
    ListTag const& left,
    ListTag const& right,
    size_t         recursionLimit = std::numeric_limits<size_t>::max()
);

// TODO: Test needed
std::pair<std::unique_ptr<CompoundTag>, std::unique_ptr<CompoundTag>> tagDiff(
    CompoundTag const& left,
    CompoundTag const& right,
    size_t             recursionLimit = std::numeric_limits<size_t>::max()
);

inline std::unique_ptr<ListTag> fromBlockPos(BlockPos const& bpos) {
    return std::make_unique<ListTag>(ListTag{bpos.x, bpos.y, bpos.z});
}
inline std::unique_ptr<ListTag> fromVec3(Vec3 const& vec) {
    return std::make_unique<ListTag>(ListTag{vec.x, vec.y, vec.z});
}
inline BlockPos toBlockPos(ListTag const& tag) { return {(int)tag[0], (int)tag[1], (int)tag[2]}; }
inline Vec3     toVec3(ListTag const& tag) {
    return {tag[0]->as<FloatTag>(), tag[1]->as<FloatTag>(), tag[2]->as<FloatTag>()};
}

} // namespace lfp::utils::nbt_utils