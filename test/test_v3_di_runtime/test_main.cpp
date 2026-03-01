#include <unity.h>

#include "../../src/kernel/v3_di_runtime.cpp"

void setUp() {}
void tearDown() {}

void test_reset_condition_inhibits_and_clears_runtime() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 0;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = false;
  runtime.triggerFlag = true;
  runtime.currentValue = 9;
  runtime.startOnMs = 50;
  runtime.startOffMs = 60;
  runtime.repeatCounter = 3;
  runtime.state = State_DI_Qualified;

  V3DiStepInput in = {};
  in.nowMs = 100;
  in.sample = true;
  in.setCondition = true;
  in.resetCondition = true;
  in.prevSample = false;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(out.setResult);
  TEST_ASSERT_TRUE(out.resetResult);
  TEST_ASSERT_TRUE(out.resetOverride);
  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_TRUE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(0, runtime.currentValue);
  TEST_ASSERT_EQUAL(State_DI_Inhibited, runtime.state);
}

void test_set_condition_false_forces_idle_and_clears_trigger_only() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 0;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 2;
  runtime.state = State_DI_Qualified;

  V3DiStepInput in = {};
  in.nowMs = 100;
  in.sample = false;
  in.setCondition = false;
  in.resetCondition = false;
  in.prevSample = true;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_FALSE(out.setResult);
  TEST_ASSERT_FALSE(out.resetResult);
  TEST_ASSERT_FALSE(out.resetOverride);
  TEST_ASSERT_TRUE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(2, runtime.currentValue);
  TEST_ASSERT_EQUAL(State_DI_Idle, runtime.state);
}

void test_rising_edge_qualifies_and_increments_counter() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 0;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.currentValue = 5;
  runtime.startOnMs = 0;
  runtime.state = State_DI_Idle;

  V3DiStepInput in = {};
  in.nowMs = 120;
  in.sample = true;
  in.setCondition = true;
  in.resetCondition = false;
  in.prevSample = false;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(runtime.logicalState);
  TEST_ASSERT_TRUE(runtime.physicalState);
  TEST_ASSERT_TRUE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(6, runtime.currentValue);
  TEST_ASSERT_EQUAL_UINT32(120, runtime.startOnMs);
  TEST_ASSERT_EQUAL(State_DI_Qualified, runtime.state);
}

void test_non_matching_edge_does_not_qualify() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 0;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.currentValue = 4;
  runtime.state = State_DI_Qualified;

  V3DiStepInput in = {};
  in.nowMs = 200;
  in.sample = false;
  in.setCondition = true;
  in.resetCondition = false;
  in.prevSample = true;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(4, runtime.currentValue);
  TEST_ASSERT_EQUAL(State_DI_Idle, runtime.state);
}

void test_debounce_window_blocks_qualification() {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = 50;
  cfg.edgeMode = Mode_DI_Rising;

  V3DiRuntimeState runtime = {};
  runtime.currentValue = 7;
  runtime.startOnMs = 100;
  runtime.state = State_DI_Idle;

  V3DiStepInput in = {};
  in.nowMs = 130;
  in.sample = true;
  in.setCondition = true;
  in.resetCondition = false;
  in.prevSample = false;
  in.prevSampleValid = true;

  V3DiStepOutput out = {};
  runV3DiStep(cfg, runtime, in, out);

  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(7, runtime.currentValue);
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
