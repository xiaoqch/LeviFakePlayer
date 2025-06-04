#pragma once

#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/FloatTag.h"
#include "mc/nbt/ListTag.h"
#include "mc/world/level/BlockPos.h"

namespace lfp::utils::nbt_utils {


bool swapChildByKey(CompoundTag& comp1, CompoundTag& comp2, std::string_view key);

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
template <std::derived_from<Tag> T>
consteval Tag::Type tagType() {
    if constexpr (std::same_as<T, EndTag>) return Tag::Type::End;
    else if constexpr (std::same_as<T, ByteTag>) return Tag::Type::Byte;
    else if constexpr (std::same_as<T, ShortTag>) return Tag::Type::Short;
    else if constexpr (std::same_as<T, IntTag>) return Tag::Type::Int;
    else if constexpr (std::same_as<T, Int64Tag>) return Tag::Type::Int64;
    else if constexpr (std::same_as<T, FloatTag>) return Tag::Type::Float;
    else if constexpr (std::same_as<T, DoubleTag>) return Tag::Type::Double;
    else if constexpr (std::same_as<T, ByteArrayTag>) return Tag::Type::ByteArray;
    else if constexpr (std::same_as<T, StringTag>) return Tag::Type::String;
    else if constexpr (std::same_as<T, ListTag>) return Tag::Type::List;
    else if constexpr (std::same_as<T, CompoundTag>) return Tag::Type::Compound;
    else if constexpr (std::same_as<T, IntArrayTag>) return Tag::Type::IntArray;
}

template <std::derived_from<Tag> T, size_t N>
inline auto listDataViewWithValidation(Tag const& tag) {
    constexpr auto takeData = [](auto& tag) { return tag->template as<T>().data; };
    using ViewT =
        std::optional<decltype(std::declval<ListTag const&>() | std::views::transform(takeData))>;

    if (tag.getId() != Tag::List) return ViewT{};
    auto& list = tag.as<ListTag>();
    if (list.size() != N || list.mType != tagType<T>()) return ViewT{};
    return ViewT{list | std::views::transform(takeData)};
}

inline std::optional<BlockPos> toBlockPos(Tag const& tag) {
    auto view = listDataViewWithValidation<IntTag, 3>(tag);
    if (!view) return {};
    return BlockPos{(*view)[0], (*view)[1], (*view)[2]};
}
inline std::optional<Vec3> toVec3(Tag const& tag) {
    auto view = listDataViewWithValidation<FloatTag, 3>(tag);
    if (!view) return {};
    return Vec3{(*view)[0], (*view)[1], (*view)[2]};
}
inline std::optional<Vec2> toVec2(Tag const& tag) {
    auto view = listDataViewWithValidation<FloatTag, 2>(tag);
    if (!view) return {};
    return Vec2{(*view)[0], (*view)[1]};
}

} // namespace lfp::utils::nbt_utils