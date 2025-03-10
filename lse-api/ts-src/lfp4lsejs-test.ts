ll.registerPlugin(
  /* name */ "lfp4lsejs-test",
  /* introduction */ "LeviFakePlayer test for LSE API ",
  /* version */ [0, 0, 1],
  /* otherInformation */ {}
);

// The purpose of this file:
// 1. LSE has an abnormal logic for searching relative import paths for entry files.
// 2. LSE may not print any errors before the direct import is completed.

mc.listen("onServerStarted", async () => {
  try {
    const testMain = (await import("plugins/lfp4lsejs-test/test-main.js"))
      .default;
    await testMain();
  } catch (error) {
    logger.error("Error occurred when testing");
    logger.error(error);
  }
});
