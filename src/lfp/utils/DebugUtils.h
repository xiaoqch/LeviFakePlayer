#pragma once

#if defined(LFP_DEBUG) && !defined(LFP_TEST_MODE)

#include <Windows.h>

#include "ll/api/io/Logger.h"

class CompoundTag;
class Player;
namespace lfp::utils::debug_utils {
extern ll::io::Logger& debugLogger;
inline double ns_time(LARGE_INTEGER begin_time, LARGE_INTEGER end_time, LARGE_INTEGER freq_) {
    return (end_time.QuadPart - begin_time.QuadPart) * 1000000.0 / freq_.QuadPart;
}
#define TestFuncTime(func, ...)                                                                    \
    {                                                                                              \
        LARGE_INTEGER freq_;                                                                       \
        LARGE_INTEGER begin_time;                                                                  \
        LARGE_INTEGER end_time;                                                                    \
        QueryPerformanceFrequency(&freq_);                                                         \
        QueryPerformanceCounter(&begin_time);                                                      \
        func(__VA_ARGS__);                                                                         \
        QueryPerformanceCounter(&end_time);                                                        \
        debugLogger.warn("{}\t time: {}ns", #func, ns_time(begin_time, end_time, freq_));          \
    }

inline void _WASSERT(_In_z_ char const* _Message, _In_z_ char const* _File, _In_ unsigned _Line) {
    debugLogger.error("断言失败，表达式：{}", _Message);
    debugLogger.error("位于文件：{}", _File);
    debugLogger.error("第 {} 行", _Line);
};

#define ASSERT(expression)                                                                         \
    (void)((!!(expression))                                                                        \
           || (_WASSERT(#expression, __FILE__, (unsigned)(__LINE__)), __debugbreak(), 0))
#define DEBUGL(...)  debugLogger.info(__VA_ARGS__)
#define DEBUGW(...)  debugLogger.warn(__VA_ARGS__)
#define LOG_VAR(var) debugLogger.info("{} = {}", #var, var);

template <size_t size = 1000, typename T = void******>
struct voids {
    T filler[size];
};
extern void debugLogNbt(CompoundTag const& tag);
extern void logPlayerInfo(Player* player);

std::string debugCompareTag(CompoundTag const& left, CompoundTag const& right);

} // namespace lfp::utils::debug_utils

using namespace lfp::utils::debug_utils;

#else
#define TestFuncTime(func, ...) ((void)0)
#define ASSERT(var)             ((void)0)
#define DEBUGL(...)             ((void)0)
#define DEBUGW(...)             ((void)0)
#define LOG_VAR(var)            ((void)0)
#define debugLogNbt(x)          ((void)0)
#define debugCompareTag(...)    ((void)0)
#endif // LFP_DEBUG
