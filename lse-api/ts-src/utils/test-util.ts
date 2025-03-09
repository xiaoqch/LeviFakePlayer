import type {} from "../api/lib";

export let TestErrorCounter = 0;
export function EXPECT_TRUE(val: any) {
  if (!val) {
    logger.error(new Error(`Test Failed: ${val} not is true`));
    ++TestErrorCounter;
  }
  return val;
}
export function EXPECT_EQ(val: any, val2: any) {
  if (val !== val2) {
    logger.error(new Error(`Test Failed: ${val} not equals to ${val2}`));
    ++TestErrorCounter;
  }
  return val;
}

export function EXPECT_NE(val: any, val2: any) {
  if (val === val2) {
    logger.error(new Error(`Test Failed: ${val} equals to ${val2}`));
    ++TestErrorCounter;
  }
  return val;
}
