#include <unity.h>

#include "../../src/kernel/v3_rtc_runtime.cpp"

void setUp() {}
void tearDown() {}

void test_rtc_step_clears_trigger_when_logical_false() {
  V3RtcRuntimeConfig cfg = {};
  cfg.triggerDurationMs = 60000;

  V3RtcRuntimeState runtime = {};
  runtime.logicalState = false;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 1;
  runtime.triggerStartMs = 100;
  runtime.mode = Mode_DO_Normal;
  runtime.state = State_DO_Active;

  V3RtcStepInput in = {};
  in.nowMs = 500;

  runV3RtcStep(cfg, runtime, in);

  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(0, runtime.currentValue);
  TEST_ASSERT_EQUAL(Mode_None, runtime.mode);
  TEST_ASSERT_EQUAL(State_None, runtime.state);
}

void test_rtc_step_holds_true_within_trigger_window() {
  V3RtcRuntimeConfig cfg = {};
  cfg.triggerDurationMs = 1000;

  V3RtcRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 1;
  runtime.triggerStartMs = 200;

  V3RtcStepInput in = {};
  in.nowMs = 900;

  runV3RtcStep(cfg, runtime, in);

  TEST_ASSERT_TRUE(runtime.logicalState);
  TEST_ASSERT_TRUE(runtime.physicalState);
  TEST_ASSERT_TRUE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(1, runtime.currentValue);
}

void test_rtc_step_auto_clears_after_trigger_window() {
  V3RtcRuntimeConfig cfg = {};
  cfg.triggerDurationMs = 1000;

  V3RtcRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 1;
  runtime.triggerStartMs = 200;

  V3RtcStepInput in = {};
  in.nowMs = 1200;

  runV3RtcStep(cfg, runtime, in);

  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(0, runtime.currentValue);
}

void test_rtc_channel_matches_minute_with_wildcards() {
  V3RtcScheduleView channel = {};
  channel.enabled = true;
  channel.year = -1;
  channel.month = 3;
  channel.day = -1;
  channel.weekday = 0;
  channel.hour = 14;
  channel.minute = 30;

  V3RtcMinuteStamp stamp = {};
  stamp.year = 2026;
  stamp.month = 3;
  stamp.day = 1;
  stamp.weekday = 0;
  stamp.hour = 14;
  stamp.minute = 30;

  TEST_ASSERT_TRUE(v3RtcChannelMatchesMinute(channel, stamp));

  stamp.minute = 31;
  TEST_ASSERT_FALSE(v3RtcChannelMatchesMinute(channel, stamp));
}

void test_rtc_minute_key_changes_per_minute() {
  V3RtcMinuteStamp a = {};
  a.year = 2026;
  a.month = 3;
  a.day = 1;
  a.weekday = 0;
  a.hour = 14;
  a.minute = 30;

  V3RtcMinuteStamp b = a;
  b.minute = 31;

  TEST_ASSERT_NOT_EQUAL(v3RtcMinuteKey(a), v3RtcMinuteKey(b));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_rtc_step_clears_trigger_when_logical_false);
  RUN_TEST(test_rtc_step_holds_true_within_trigger_window);
  RUN_TEST(test_rtc_step_auto_clears_after_trigger_window);
  RUN_TEST(test_rtc_channel_matches_minute_with_wildcards);
  RUN_TEST(test_rtc_minute_key_changes_per_minute);
  return UNITY_END();
}
