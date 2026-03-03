#include <unity.h>

#include "../../src/kernel/v3_di_runtime.cpp"

void setUp() {}
void tearDown() {}

void test_reset_condition_inhibits_and_clears_runtime() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 0;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.commandState = true;
  runtime.actualState = false;
  runtime.edgePulse = true;
  runtime.liveValue = 9;
  runtime.startOnMs = 50;
  runtime.startOffMs = 60;
  runtime.repeatCounter = 3;
  runtime.state = State_DI_Qualified;

  V3DiStepInput in = {};
  in.nowMs = 100;
  in.sample = true;
  in.turnOnCondition = true;
  in.turnOffCondition = true;
  in.prevSample = false;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(out.turnOnConditionMet);
  TEST_ASSERT_TRUE(out.turnOffConditionMet);
  TEST_ASSERT_TRUE(out.resetOverride);
  TEST_ASSERT_FALSE(runtime.commandState);
  TEST_ASSERT_TRUE(runtime.actualState);
  TEST_ASSERT_FALSE(runtime.edgePulse);
  TEST_ASSERT_EQUAL_UINT32(0, runtime.liveValue);
  TEST_ASSERT_EQUAL(State_DI_Inhibited, runtime.state);
}

void test_set_condition_false_forces_idle_and_clears_trigger_only() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 0;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.commandState = true;
  runtime.edgePulse = true;
  runtime.liveValue = 2;
  runtime.state = State_DI_Qualified;

  V3DiStepInput in = {};
  in.nowMs = 100;
  in.sample = false;
  in.turnOnCondition = false;
  in.turnOffCondition = false;
  in.prevSample = true;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_FALSE(out.turnOnConditionMet);
  TEST_ASSERT_FALSE(out.turnOffConditionMet);
  TEST_ASSERT_FALSE(out.resetOverride);
  TEST_ASSERT_TRUE(runtime.commandState);
  TEST_ASSERT_FALSE(runtime.actualState);
  TEST_ASSERT_FALSE(runtime.edgePulse);
  TEST_ASSERT_EQUAL_UINT32(2, runtime.liveValue);
  TEST_ASSERT_EQUAL(State_DI_Idle, runtime.state);
}

void test_rising_edge_qualifies_and_increments_counter() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 0;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.liveValue = 5;
  runtime.startOnMs = 0;
  runtime.state = State_DI_Idle;

  V3DiStepInput in = {};
  in.nowMs = 120;
  in.sample = true;
  in.turnOnCondition = true;
  in.turnOffCondition = false;
  in.prevSample = false;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(runtime.commandState);
  TEST_ASSERT_TRUE(runtime.actualState);
  TEST_ASSERT_TRUE(runtime.edgePulse);
  TEST_ASSERT_EQUAL_UINT32(6, runtime.liveValue);
  TEST_ASSERT_EQUAL_UINT32(120, runtime.startOnMs);
  TEST_ASSERT_EQUAL(State_DI_Qualified, runtime.state);
}

void test_non_matching_edge_does_not_qualify() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 0;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.commandState = true;
  runtime.liveValue = 4;
  runtime.state = State_DI_Qualified;

  V3DiStepInput in = {};
  in.nowMs = 200;
  in.sample = false;
  in.turnOnCondition = true;
  in.turnOffCondition = false;
  in.prevSample = true;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(runtime.commandState);
  TEST_ASSERT_FALSE(runtime.actualState);
  TEST_ASSERT_FALSE(runtime.edgePulse);
  TEST_ASSERT_EQUAL_UINT32(4, runtime.liveValue);
  TEST_ASSERT_EQUAL(State_DI_Idle, runtime.state);
}

void test_debounce_window_blocks_qualification() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 50;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.liveValue = 7;
  runtime.startOnMs = 100;
  runtime.state = State_DI_Idle;

  V3DiStepInput in = {};
  in.nowMs = 130;
  in.sample = true;
  in.turnOnCondition = true;
  in.turnOffCondition = false;
  in.prevSample = false;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_FALSE(runtime.edgePulse);
  TEST_ASSERT_EQUAL_UINT32(7, runtime.liveValue);
  TEST_ASSERT_EQUAL_UINT32(100, runtime.startOnMs);
  TEST_ASSERT_EQUAL(State_DI_Filtering, runtime.state);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_reset_condition_inhibits_and_clears_runtime);
  RUN_TEST(test_set_condition_false_forces_idle_and_clears_trigger_only);
  RUN_TEST(test_rising_edge_qualifies_and_increments_counter);
  RUN_TEST(test_non_matching_edge_does_not_qualify);
  RUN_TEST(test_debounce_window_blocks_qualification);
  return UNITY_END();
}



