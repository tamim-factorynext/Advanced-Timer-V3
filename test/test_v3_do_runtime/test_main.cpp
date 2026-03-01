#include <unity.h>

#include "../../src/kernel/v3_do_runtime.cpp"

void setUp() {}
void tearDown() {}

void test_reset_condition_forces_idle_and_clears_counter() {
  V3DoRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Normal;
  cfg.delayBeforeOnMs = 50;
  cfg.onDurationMs = 100;
  cfg.repeatCount = 2;

  V3DoRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 7;
  runtime.startOnMs = 11;
  runtime.startOffMs = 22;
  runtime.repeatCounter = 1;
  runtime.state = State_DO_Active;

  V3DoStepInput in = {};
  in.nowMs = 100;
  in.setCondition = true;
  in.resetCondition = true;

  V3DoStepOutput out = {};
  runV3DoStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(out.setResult);
  TEST_ASSERT_TRUE(out.resetResult);
  TEST_ASSERT_TRUE(out.resetOverride);
  TEST_ASSERT_FALSE(out.effectiveOutput);
  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(0, runtime.currentValue);
  TEST_ASSERT_EQUAL(State_DO_Idle, runtime.state);
}

void test_immediate_mode_triggers_active_output() {
  V3DoRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Immediate;
  cfg.delayBeforeOnMs = 50;
  cfg.onDurationMs = 100;
  cfg.repeatCount = 1;

  V3DoRuntimeState runtime = {};
  runtime.state = State_DO_Idle;

  V3DoStepInput in = {};
  in.nowMs = 100;
  in.setCondition = true;
  in.resetCondition = false;

  V3DoStepOutput out = {};
  runV3DoStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(runtime.logicalState);
  TEST_ASSERT_TRUE(runtime.physicalState);
  TEST_ASSERT_TRUE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL(State_DO_Active, runtime.state);
  TEST_ASSERT_EQUAL_UINT32(100, runtime.startOffMs);
  TEST_ASSERT_EQUAL_UINT32(1, runtime.currentValue);
  TEST_ASSERT_TRUE(out.effectiveOutput);
}

void test_normal_mode_transitions_ondelay_to_active_after_delay() {
  V3DoRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Normal;
  cfg.delayBeforeOnMs = 50;
  cfg.onDurationMs = 100;
  cfg.repeatCount = 1;

  V3DoRuntimeState runtime = {};
  runtime.state = State_DO_Idle;

  V3DoStepInput in = {};
  in.nowMs = 10;
  in.setCondition = true;
  in.resetCondition = false;

  V3DoStepOutput out = {};
  runV3DoStep(cfg, runtime, in, out);

  TEST_ASSERT_EQUAL(State_DO_OnDelay, runtime.state);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(out.effectiveOutput);
  TEST_ASSERT_EQUAL_UINT32(0, runtime.currentValue);

  in.nowMs = 70;
  runV3DoStep(cfg, runtime, in, out);

  TEST_ASSERT_EQUAL(State_DO_Active, runtime.state);
  TEST_ASSERT_TRUE(runtime.physicalState);
  TEST_ASSERT_TRUE(out.effectiveOutput);
  TEST_ASSERT_EQUAL_UINT32(1, runtime.currentValue);
  TEST_ASSERT_EQUAL_UINT32(70, runtime.startOffMs);
}

void test_active_window_completes_to_finished_at_repeat_limit() {
  V3DoRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Normal;
  cfg.delayBeforeOnMs = 50;
  cfg.onDurationMs = 100;
  cfg.repeatCount = 1;

  V3DoRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.currentValue = 3;
  runtime.startOffMs = 0;
  runtime.repeatCounter = 0;
  runtime.state = State_DO_Active;

  V3DoStepInput in = {};
  in.nowMs = 120;
  in.setCondition = true;
  in.resetCondition = false;

  V3DoStepOutput out = {};
  runV3DoStep(cfg, runtime, in, out);

  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_EQUAL_UINT32(1, runtime.repeatCounter);
  TEST_ASSERT_EQUAL(State_DO_Finished, runtime.state);
  TEST_ASSERT_FALSE(out.effectiveOutput);
  TEST_ASSERT_EQUAL_UINT32(3, runtime.currentValue);
}

void test_gated_mode_drops_to_idle_when_gate_false() {
  V3DoRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Gated;
  cfg.delayBeforeOnMs = 20;
  cfg.onDurationMs = 100;
  cfg.repeatCount = 2;

  V3DoRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 5;
  runtime.startOnMs = 33;
  runtime.startOffMs = 44;
  runtime.repeatCounter = 1;
  runtime.state = State_DO_Active;

  V3DoStepInput in = {};
  in.nowMs = 200;
  in.setCondition = false;
  in.resetCondition = false;

  V3DoStepOutput out = {};
  runV3DoStep(cfg, runtime, in, out);

  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(5, runtime.currentValue);
  TEST_ASSERT_EQUAL_UINT32(0, runtime.repeatCounter);
  TEST_ASSERT_EQUAL(State_DO_Idle, runtime.state);
  TEST_ASSERT_FALSE(out.effectiveOutput);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_reset_condition_forces_idle_and_clears_counter);
  RUN_TEST(test_immediate_mode_triggers_active_output);
  RUN_TEST(test_normal_mode_transitions_ondelay_to_active_after_delay);
  RUN_TEST(test_active_window_completes_to_finished_at_repeat_limit);
  RUN_TEST(test_gated_mode_drops_to_idle_when_gate_false);
  return UNITY_END();
}

