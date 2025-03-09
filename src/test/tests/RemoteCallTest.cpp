#include "../TestManager.h"

#include "lfp/api/rcapi/rcapi.h"


namespace lfp::test {

LFP_TEST(RemoteCallTest, Api) {
    auto&       manager = lfp::FakePlayerManager::getManager();
    std::string fpName  = mSetName + mTestName;
    lfp::api::rcapi::create(fpName);
    auto fp = manager.tryGetFakePlayer(fpName);
    EXPECT_TRUE(fp);
    lfp::api::rcapi::login(fpName);
    EXPECT_TRUE(fp->isOnline());
    lfp::api::rcapi::logout(fpName);
    EXPECT_TRUE(!fp->isOnline());
    lfp::api::rcapi::remove(fpName);
    EXPECT_TRUE(!manager.tryGetFakePlayer(fpName));
}

LFP_TEST(RemoteCallTest, RemoteApi) {
    EXPECT_TRUE(ImportFakePlayerAPI(list)() == lfp::api::rcapi::list());
    EXPECT_TRUE(ImportFakePlayerAPI(getOnlineList)() == lfp::api::rcapi::getOnlineList());

    auto&       manager = lfp::FakePlayerManager::getManager();
    std::string fpName  = mSetName + mTestName;
    ImportFakePlayerAPI(create)(fpName);
    auto fp = manager.tryGetFakePlayer(fpName);
    EXPECT_TRUE(fp);
    ImportFakePlayerAPI(login)(fpName);
    EXPECT_TRUE(fp->isOnline());
    ImportFakePlayerAPI(logout)(fpName);
    EXPECT_TRUE(!fp->isOnline());
    ImportFakePlayerAPI(remove)(fpName);
    EXPECT_TRUE(!manager.tryGetFakePlayer(fpName));
}

} // namespace lfp::test
