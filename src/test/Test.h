#pragma once

#include <string>
#include <string_view>

#include "ll/api/coro/CoroTask.h"

namespace lfp::test {

class Test {
public:
    std::string mSetName;
    std::string mTestName;

    struct TestResult {
        bool        success        = true;
        std::string failureMessage = "";
    } mResult;

    using MayBeCoroTaskType = std::optional<ll::coro::CoroTask<void>>;
    virtual ~Test()         = default;
    Test(std::string_view setName, std::string_view testName)
    : mSetName(setName),
      mTestName(testName) {}
    [[nodiscard]] virtual MayBeCoroTaskType runThis() = 0;
    void onFailure(std::string_view _Message, _In_z_ char const* _File, _In_ unsigned _Line) {
        mResult.failureMessage +=
            fmt::format("\n测试失败：{}\n位于文件： {}\n第 {} 行", _Message, _File, _Line);
        mResult.success = false;
    }
    [[nodiscard]] std::string      fullName() { return mSetName + "::" + mTestName; }
    [[nodiscard]] bool             isSuccess() { return mResult.success; }
    [[nodiscard]] std::string_view failureMessage() { return mResult.failureMessage; }
};

class SyncTest : public Test {
public:
    using ResultType    = void;
    virtual ~SyncTest() = default;
    SyncTest(std::string_view setName, std::string_view testName) : Test(setName, testName) {}
    [[nodiscard]] virtual Test::MayBeCoroTaskType runThis() override {
        runThisImpl();
        return {};
    };

protected:
    [[noreturn]] virtual ResultType runThisImpl() = 0;
};

class AsyncTest : public Test {
public:
    using ResultType = ll::coro::CoroTask<void>;

    virtual ~AsyncTest() = default;
    AsyncTest(std::string_view setName, std::string_view testName) : Test(setName, testName) {}
    [[nodiscard]] virtual Test::MayBeCoroTaskType runThis() override { return runThisImpl(); };

protected:
    [[nodiscard]] virtual ResultType runThisImpl() = 0;
};

} // namespace lfp::test