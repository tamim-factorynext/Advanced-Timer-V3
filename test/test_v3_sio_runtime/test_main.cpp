#include <unity.h>

#include "../../src/kernel/v3_do_runtime.cpp"
#include "../../src/kernel/v3_sio_runtime.cpp"

void setUp() {}
void tearDown() {}

void test_sio_reset_condition_forces_idle() {
  V3SioRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Normal;
  cfg.delayBeforeOnMs = 20;
  cfg.onDurationMs = 40;
  cfg.repeatCount = 1;

  V3SioRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 9;
  runtime.repeatCounter = 1;
  runtime.state = State_DO_Active;

  V3SioStepInput in = {};
  in.nowMs = 100;
  in.setCondition = true;
  in.resetCondition = true;

  V3SioStepOutput out = {};
  runV3SioStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(out.resetResult);
  TEST_ASSERT_TRUE(out.resetOverride);
  TEST_ASSERT_EQUAL(State_DO_Idle, runtime.state);
  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_EQUAL_UINT32(0, runtime.currentValue);
}

void test_sio_immediate_enters_active_mission_state() {
  V3SioRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Immediate;
  cfg.delayBeforeOnMs = 0;
  cfg.onDurationMs = 100;
  cfg.repeatCount = 1;

  V3SioRuntimeState runtime = {};
  runtime.state = State_DO_Idle;

  V3SioStepInput in = {};
  in.nowMs = 10;
  in.setCondition = true;
  in.resetCondition = false;

  V3SioStepOutput out = {};
  runV3SioStep(cfg, runtime, in, out);

  TEST_ASSERT_EQUAL(State_DO_Active, runtime.state);
  TEST_ASSERT_TRUE(runtime.logicalState);
  TEST_ASSERT_TRUE(runtime.physicalState);
  TEST_ASSERT_TRUE(out.effectiveOutput);
}

void test_sio_normal_finishes_at_repeat_limit() {
  V3SioRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Normal;
  cfg.delayBeforeOnMs = 20;
  cfg.onDurationMs = 50;
  cfg.repeatCount = 1;

  V3SioRuntimeState runtime = {};
  runtime.state = State_DO_Idle;

  V3SioStepInput in = {};
  in.nowMs = 0;
  in.setCondition = true;
  in.resetCondition = false;

  V3SioStepOutput out = {};
  runV3SioStep(cfg, runtime, in, out);
  TEST_ASSERT_EQUAL(State_DO_OnDelay, runtime.state);

  in.nowMs = 25;
  runV3SioStep(cfg, runtime, in, out);
  TEST_ASSERT_EQUAL(State_DO_Active, runtime.state);

  in.nowMs = 90;
  runV3SioStep(cfg, runtime, in, out);
  TEST_ASSERT_EQUAL(State_DO_Finished, runtime.state);
  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
}

void test_sio_gated_drops_to_idle_when_gate_false() {
  V3SioRuntimeConfig cfg = {};
  cfg.mode = Mode_DO_Gated;
  cfg.delayBeforeOnMs = 20;
  cfg.onDurationMs = 100;
  cfg.repeatCount = 3;

  V3SioRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 4;
  runtime.repeatCounter = 2;
  runtime.state = State_DO_Active;

  V3SioStepInput in = {};
  in.nowMs = 200;
  in.setCondition = false;
  in.resetCondition = false;

  V3SioStepOutput out = {};
  runV3SioStep(cfg, runtime, in, out);

  TEST_ASSERT_EQUAL(State_DO_Idle, runtime.state);
  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(4, runtime.currentValue);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sio_reset_condition_forces_idle);
  RUN_TEST(test_sio_immediate_enters_active_mission_state);
  RUN_TEST(test_sio_normal_finishes_at_repeat_limit);
  RUN_TEST(test_sio_gated_drops_to_idle_when_gate_false);
  return UNITY_END();
}
