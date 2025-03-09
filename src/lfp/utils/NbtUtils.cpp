#include "NbtUtils.h"
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <utility>
#include <vector>

#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/EndTag.h"
#include "mc/nbt/ListTag.h"


namespace lfp::utils::nbt_utils {

bool swapChildByKey(CompoundTag& comp1, CompoundTag& comp2, std::string_view key) {
    // comp1.mTags.find(key)->second.mTagStorage.swap(comp2.mTags.find(key)->second.mTagStorage);
    auto iter1 = comp1.mTags.find(key);
    auto iter2 = comp2.mTags.find(key);
    auto has1  = comp1.contains(key);
    auto has2  = comp2.contains(key);
    if (!has1 && !has2) return false;
    if (has1)
        if (has2) iter1->second.mTagStorage.swap(iter2->second.mTagStorage);
        else comp2.mTags.insert(comp1.mTags.extract(iter1));
    else comp1.mTags.insert(comp2.mTags.extract(iter2));
    return true;
}

template <std::derived_from<Tag> T, std::derived_from<Tag> T2, typename K>
    requires(requires(T2 ctn, K key) { ctn[key]; })
bool _tagDiff(
    T const& left,
    T const& right,
    T2&      leftContainer,
    T2&      rightContainer,
    K        key,
    size_t   recursionLimit
) {
    T leftOut{}, rightOut{};
    if (tagDiff(left, right, leftOut, rightOut, recursionLimit)) {
        if constexpr (std::is_same_v<T2, ListTag>) {
            leftContainer.emplace_back(std::move(leftOut));
            rightContainer.emplace_back(std::move(rightOut));
        } else {
            leftContainer[key]  = std::move(leftOut);
            rightContainer[key] = std::move(rightOut);
        }
        return false;
    }
    return false;
}

template <std::derived_from<Tag> Item>
bool _ComplexListDiff(
    ListTag const& left,
    ListTag const& right,
    ListTag&       leftOut,
    ListTag&       rightOut,
    size_t         recursionLimit
) {
    auto leftSize               = left.size();
    auto rightSize              = right.size();
    auto [minLength, maxLength] = std::minmax(leftSize, rightSize);
    bool difference             = false;
    for (auto index : std::views::iota(0, minLength)) {
        if (left[index]->equals(*right[index])) continue;
        difference = true;
        if (recursionLimit > 0) {
            _tagDiff(
                left[index]->as<Item>(),
                right[index]->as<Item>(),
                leftOut,
                rightOut,
                index,
                recursionLimit - 1
            );
        } else {
            leftOut.push_back(left[index]);
            rightOut.push_back(right[index]);
        }
    }
    for (auto index : std::views::iota(minLength, maxLength)) {
        difference = true;
        if (index < leftSize) {
            leftOut.push_back(left[index]);
        } else {
            rightOut.push_back(right[index]);
        }
    }
    return difference;
}

bool tagDiff(
    ListTag const& left,
    ListTag const& right,
    ListTag&       leftOut,
    ListTag&       rightOut,
    size_t         recursionLimit
) {
    if (left.equals(right)) return false;

    size_t leftSize             = left.size();
    size_t rightSize            = right.size();
    auto [minLength, maxLength] = std::minmax(leftSize, rightSize);

    if (minLength > 0 && left.mType == right.mType) {
        if (left.mType == Tag::Type::Compound) {
            return _ComplexListDiff<CompoundTag>(left, right, leftOut, rightOut, recursionLimit);
        } else if (left.mType == Tag::Type::List) {
            return _ComplexListDiff<ListTag>(left, right, leftOut, rightOut, recursionLimit);
        }
    }
    // Not compare for simple list
    leftOut  = left;
    rightOut = right;
    return true;
}

bool tagDiff(
    CompoundTag const& left,
    CompoundTag const& right,
    CompoundTag&       leftOut,
    CompoundTag&       rightOut,
    size_t             recursionLimit
) {
    if (left.equals(right)) return false;

    std::unordered_set<std::string> keys{};
    for (auto& [key, val] : left.mTags) {
        keys.insert(key);
    }
    for (auto& [key, val] : right.mTags) {
        keys.insert(key);
    }

    for (auto& key : keys) {
        auto leftIter  = left.mTags.find(key);
        auto rightIter = right.mTags.find(key);
        auto hasLeft   = leftIter != left.mTags.end();
        auto hasRight  = rightIter != right.mTags.end();
        if (hasLeft && hasRight) {
            auto& leftTag  = leftIter->second.get();
            auto& rightTag = rightIter->second.get();
            if (leftTag.equals(rightTag)) {
                continue;
            }
            if (leftTag.getId() == Tag::Type::Compound && rightTag.getId() == Tag::Type::Compound
                && recursionLimit > 0) {
                _tagDiff(
                    leftTag.as<CompoundTag>(),
                    rightTag.as<CompoundTag>(),
                    leftOut,
                    rightOut,
                    key,
                    recursionLimit - 1
                );
                continue;
            } else if (leftTag.getId() == Tag::Type::List && rightTag.getId() == Tag::Type::List
                       && recursionLimit > 0) {
                _tagDiff(
                    leftTag.as<ListTag>(),
                    rightTag.as<ListTag>(),
                    leftOut,
                    rightOut,
                    key,
                    recursionLimit - 1
                );
                continue;
            }
        }
        leftOut[key]  = hasLeft ? leftIter->second : CompoundTagVariant(EndTag());
        rightOut[key] = hasRight ? rightIter->second : CompoundTagVariant(EndTag());
    }
    return true;
} // namespace lfp::utils::nbt_utils

std::pair<std::unique_ptr<ListTag>, std::unique_ptr<ListTag>>
tagDiff(ListTag const& left, ListTag const& right, size_t recursionLimit) {
    auto result = std::pair{std::make_unique<ListTag>(), std::make_unique<ListTag>()};
    if (tagDiff(left, right, *result.first, *result.second, recursionLimit)) {
        return result;
    }
    return {};
}

std::pair<std::unique_ptr<CompoundTag>, std::unique_ptr<CompoundTag>>
tagDiff(CompoundTag const& left, CompoundTag const& right, size_t recursionLimit) {
    auto result = std::pair{std::make_unique<CompoundTag>(), std::make_unique<CompoundTag>()};
    if (tagDiff(left, right, *result.first, *result.second, recursionLimit)) {
        return result;
    }
    return {};
}

} // namespace lfp::utils::nbt_utils