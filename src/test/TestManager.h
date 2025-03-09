#pragma once

#include "Test.h"
#include "utils/TestUtils.h"

#include "fmt/core.h"

namespace lfp::test {

// Specific what test set
#define TEST_THIS_SET_ONLY "FakePlayerSwapTest"
#undef TEST_THIS_SET_ONLY

class TestManager {
    std::vector<std::unique_ptr<Test>> mTests;

public:
    static TestManager& getInstance() {
        static TestManager instance;
        return instance;
    }

    template <typename T, typename... Args>
        requires(requires(Args... args) {
            std::derived_from<T, Test>;
            T(args...);
        })
    Test* registerTest(Args... args) {
        return mTests.emplace_back(std::make_unique<T>(args...)).get();
    }

    ll::coro::CoroTask<bool> runAllTest();
};


#define LFP_TEST_IMPL(setName, testName, ParentType)                                               \
    class setName##_##testName##_Test : public ::lfp::test::ParentType {                           \
    public:                                                                                        \
        virtual ~setName##_##testName##_Test() = default;                                          \
        setName##_##testName##_Test() : ::lfp::test::ParentType(#setName, #testName) {}            \
                                                                                                   \
    private:                                                                                       \
        ::lfp::test::ParentType::ResultType runThisImpl() override;                                \
    };                                                                                             \
    [[maybe_unused]] ::lfp::test::Test* setName##_##testName##_test =                              \
        ::lfp::test::TestManager::getInstance().registerTest<setName##_##testName##_Test>();       \
    ::lfp::test::ParentType::ResultType setName##_##testName##_Test::runThisImpl()

#define LFP_TEST(setName, testName)    LFP_TEST_IMPL(setName, testName, SyncTest)
#define LFP_CO_TEST(setName, testName) LFP_TEST_IMPL(setName, testName, AsyncTest)

inline std::string _exceptFormat(std::string_view exp) {
    return fmt::format("Test failed: {} is not true.", exp);
}

inline void onFailure(std::string_view _Message, _In_z_ char const* _File, _In_ unsigned _Line) {
    testLogger.error("测试失败：{}\n位于文件： {}\n第 {} 行", _Message, _File, _Line);
}

#define EXPECT_TRUE(expression)                                                                    \
    (void)((!!(expression))                                                                        \
           || (onFailure(::lfp::test::_exceptFormat(#expression), __FILE__, (unsigned)(__LINE__)), \
               __debugbreak(),                                                                     \
               0))


} // namespace lfp::test