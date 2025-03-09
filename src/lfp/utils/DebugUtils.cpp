
#if defined(LFP_DEBUG) && !defined(LFP_TEST_MODE)
#include "DebugUtils.h"

#include "magic_enum.hpp"
#include "mc/deps/core/math/Vec3.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/world/actor/player/Player.h"

#include "ll/api/io/Logger.h"

#include "lfp/LeviFakePlayer.h"
#include "lfp/utils/ColorUtils.h"

namespace lfp::utils::debug_utils {

ll::io::Logger& debugLogger = lfp::LeviFakePlayer::getLogger();
void            debugLogNbt(CompoundTag const& tag) {
    return;
    auto snbt = tag.toSnbt(SnbtFormat::PrettyConsolePrint);
    debugLogger.info("Snbt: \n{}", snbt);
}

void logPlayerInfo(Player* player) {
    DEBUGW("FakePlayer: {}", player->getNameTag());
    DEBUGW(
        "Dimension: {}, Position: ({})",
        (int)player->getDimensionId(),
        player->getPosition().toString()
    );
    std::unique_ptr<CompoundTag> tag = std::make_unique<CompoundTag>();
    player->save(*tag);
    for (auto& [k, v] : tag->mTags) {
        auto&       child = v.get();
        std::string value;
        switch (child.getId()) {
        case Tag::Byte:
            value = (int)child.as<ByteTag>().data;
            break;
        case Tag::End:
        case Tag::Short:
        case Tag::Int:
        case Tag::Int64:
        case Tag::Float:
        case Tag::Double:
        case Tag::ByteArray:
        case Tag::String:
        case Tag::Compound:
        case Tag::IntArray:
            value = child.toString();
            break;
        case Tag::List: {
            value         = "[";
            auto& listTag = child.as<ListTag>();
            for (auto& tagi : listTag) {
                value += tagi->toString() + ", ";
            }
            value += "]";
            break;
        }
        default:
            break;
        }
        DEBUGL("{}: {}", k, value);
    }
}

std::string debugCompareTag(CompoundTag const& left, CompoundTag const& right) {
    debugLogNbt(left);
    debugLogNbt(right);
    std::unordered_set<std::string> keys;
    std::ostringstream              oss;
    for (auto& [key, val] : left.mTags) {
        keys.insert(key);
    }
    for (auto& [key, val] : right.mTags) {
        keys.insert(key);
    }
    for (auto& key : keys) {
        auto& leftTag  = left.at(key).get();
        auto& rightTag = right.at(key).get();
        if (!&leftTag || !&rightTag || !leftTag.equals(rightTag)) {
            if (&leftTag) {
                auto type = leftTag.getId();
                DEBUGW("{}({})", magic_enum::enum_name(type), (int)type);
                if (type == Tag::Compound) {
                    auto k = lfp::utils::color_utils::gold(key);
                    DEBUGL(lfp::utils::color_utils::convertToConsole(k));
                    debugLogNbt(leftTag.as<CompoundTag>());
                }
            }
            if (&rightTag) {
                auto type = rightTag.getId();
                DEBUGW("{}({})", magic_enum::enum_name(type), (int)type);
                if (type == Tag::Compound) {
                    auto k = lfp::utils::color_utils::gold(key);
                    DEBUGL(lfp::utils::color_utils::convertToConsole(k));
                    debugLogNbt(rightTag.as<CompoundTag>());
                }
            }
            if (&leftTag && &rightTag && (leftTag.getId() == Tag::Compound)
                && (rightTag.getId() == Tag::Compound)) {
                oss << lfp::utils::color_utils::gold(key) << std::endl
                    << debugCompareTag(leftTag.as<CompoundTag>(), rightTag.as<CompoundTag>())
                    << std::endl;
            } else
                oss << lfp::utils::color_utils::green(key) << " - "
                    << lfp::utils::color_utils::green("Left") << ": "
                    << (!&leftTag ? "nullptr" : leftTag.toString()) << ", "
                    << lfp::utils::color_utils::green("Right") << ": "
                    << (!&rightTag ? "nullptr" : rightTag.toString()) << std::endl;
        }
    }
    auto str = oss.str();
    return lfp::utils::color_utils::convertToConsole(str);
}
} // namespace lfp::utils::debug_utils
#endif
