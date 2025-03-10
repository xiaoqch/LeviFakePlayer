import LeviFakePlayerAPI, { FakePlayerInfo } from "./api/lfp-api.js";
import {
  EXPECT_EQ,
  EXPECT_NE,
  EXPECT_TRUE,
  TestErrorCounter,
} from "./utils/test-util.js";

const wait = (ticks: number) =>
  new Promise((resolve) => setTimeout(resolve, ticks * 50));

const TEST_NAME = "TEST_LSE_API";

let eventCounter = 0;
function listener(name: string, info: FakePlayerInfo) {
  if (name == TEST_NAME) {
    EXPECT_TRUE(info.name == TEST_NAME);
    ++eventCounter;
  } else {
    // lfp native mode test
    logger.debug("Unknown event accepted", name, info);
  }
}

// LogLevel.DEBUG
logger.setLogLevel(5);

export default async function testMain() {
  await wait(1 * 20);

  logger.info("LeviFakePlayer api for lse test started");
  EXPECT_TRUE(LeviFakePlayerAPI.getVersion().length > 0);
  LeviFakePlayerAPI.remove(TEST_NAME);

  // Create
  let listenerId = EXPECT_TRUE(
    LeviFakePlayerAPI.subscribeEvent("create", listener)
  );
  const list = LeviFakePlayerAPI.list();
  EXPECT_NE(list, undefined);
  EXPECT_NE(LeviFakePlayerAPI.create(TEST_NAME), undefined);
  EXPECT_EQ(LeviFakePlayerAPI.list().length, list.length + 1);
  EXPECT_EQ(
    Object.keys(LeviFakePlayerAPI.getAllInfo()).length,
    list.length + 1
  );
  EXPECT_EQ(LeviFakePlayerAPI.getInfo(TEST_NAME)?.name, TEST_NAME);

  EXPECT_EQ(eventCounter, 1);
  EXPECT_TRUE(LeviFakePlayerAPI.unsubscribeEvent(listenerId));

  // Login
  listenerId = EXPECT_TRUE(LeviFakePlayerAPI.subscribeEvent("login", listener));
  EXPECT_TRUE(LeviFakePlayerAPI.login(TEST_NAME));
  EXPECT_EQ(LeviFakePlayerAPI.getInfo(TEST_NAME)?.online, true);
  const onlineList = LeviFakePlayerAPI.getOnlineList();
  EXPECT_NE(
    onlineList.findIndex((pl) => pl.name == TEST_NAME),
    -1
  );

  EXPECT_EQ(eventCounter, 2);
  EXPECT_TRUE(LeviFakePlayerAPI.unsubscribeEvent(listenerId));

  // Set auto login
  listenerId = EXPECT_TRUE(
    LeviFakePlayerAPI.subscribeEvent("update", listener)
  );
  EXPECT_TRUE(LeviFakePlayerAPI.setAutoLogin(TEST_NAME, true));
  EXPECT_EQ(LeviFakePlayerAPI.getInfo(TEST_NAME)?.autoLogin, true);

  EXPECT_EQ(eventCounter, 3);
  EXPECT_TRUE(LeviFakePlayerAPI.unsubscribeEvent(listenerId));

  await wait(1);

  // Logout
  listenerId = EXPECT_TRUE(
    LeviFakePlayerAPI.subscribeEvent("logout", listener)
  );
  EXPECT_TRUE(LeviFakePlayerAPI.logout(TEST_NAME));
  EXPECT_EQ(LeviFakePlayerAPI.getInfo(TEST_NAME)?.online, false);

  EXPECT_EQ(eventCounter, 4);
  EXPECT_TRUE(LeviFakePlayerAPI.unsubscribeEvent(listenerId));

  // Remove
  listenerId = EXPECT_TRUE(
    LeviFakePlayerAPI.subscribeEvent("remove", listener)
  );
  EXPECT_TRUE(LeviFakePlayerAPI.remove(TEST_NAME));
  EXPECT_EQ(Object.keys(LeviFakePlayerAPI.getAllInfo()).length, list.length);
  EXPECT_EQ(eventCounter, 5);
  EXPECT_TRUE(LeviFakePlayerAPI.unsubscribeEvent(listenerId));

  await wait(1);

  // Unsubscribe Test
  LeviFakePlayerAPI.create(TEST_NAME);
  LeviFakePlayerAPI.remove(TEST_NAME);
  EXPECT_EQ(eventCounter, 5);

  if (TestErrorCounter == 0) {
    colorLog("green", "All LSE-QuickJs API Tests Passed");
  } else {
    logger.error(
      `LSE-QuickJs API Test Failed!!!, error count:${TestErrorCounter}`
    );
  }
}
