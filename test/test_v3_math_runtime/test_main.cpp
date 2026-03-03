#include <unity.h>

#include "../../src/kernel/v3_math_runtime.cpp"

void setUp() {}
void tearDown() {}

void test_math_reset_uses_fallback_and_clears_flags() {
  V3MathRuntimeConfig cfg = {};
  cfg.inputA = 10;
  cfg.inputB = 20;
  cfg.fallbackValue = 77;
  cfg.clampMin = 0;
  cfg.clampMax = 100;
  cfg.clampEnabled = true;

  V3MathRuntimeState runtime = {};
  runtime.commandState = true;
  runtime.actualState = true;
  runtime.edgePulse = true;
  runtime.liveValue = 500;
  runtime.state = State_DO_Active;

  V3MathStepInput in = {};
  in.turnOnCondition = true;
  in.turnOffCondition = true;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(out.turnOnConditionMet);
  TEST_ASSERT_TRUE(out.turnOffConditionMet);
  TEST_ASSERT_TRUE(out.resetOverride);
  TEST_ASSERT_FALSE(runtime.commandState);
  TEST_ASSERT_FALSE(runtime.actualState);
  TEST_ASSERT_FALSE(runtime.edgePulse);
  TEST_ASSERT_EQUAL_UINT32(77, runtime.liveValue);
  TEST_ASSERT_EQUAL(State_None, runtime.state);
}

void test_math_no_set_clears_flags_keeps_current_value() {
  V3MathRuntimeConfig cfg = {};
  cfg.inputA = 1;
  cfg.inputB = 2;
  cfg.fallbackValue = 9;

  V3MathRuntimeState runtime = {};
  runtime.commandState = true;
  runtime.actualState = true;
  runtime.edgePulse = true;
  runtime.liveValue = 1234;
  runtime.state = State_DO_Active;

  V3MathStepInput in = {};
  in.turnOnCondition = false;
  in.turnOffCondition = false;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_FALSE(out.turnOnConditionMet);
  TEST_ASSERT_FALSE(out.turnOffConditionMet);
  TEST_ASSERT_FALSE(out.resetOverride);
  TEST_ASSERT_FALSE(runtime.commandState);
  TEST_ASSERT_FALSE(runtime.actualState);
  TEST_ASSERT_FALSE(runtime.edgePulse);
  TEST_ASSERT_EQUAL_UINT32(1234, runtime.liveValue);
  TEST_ASSERT_EQUAL(State_None, runtime.state);
}

void test_math_set_computes_sum_without_clamp() {
  V3MathRuntimeConfig cfg = {};
  cfg.inputA = 7;
  cfg.inputB = 11;
  cfg.fallbackValue = 0;
  cfg.clampEnabled = false;

  V3MathRuntimeState runtime = {};

  V3MathStepInput in = {};
  in.turnOnCondition = true;
  in.turnOffCondition = false;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(runtime.commandState);
  TEST_ASSERT_TRUE(runtime.actualState);
  TEST_ASSERT_TRUE(runtime.edgePulse);
  TEST_ASSERT_EQUAL_UINT32(18, runtime.liveValue);
  TEST_ASSERT_EQUAL(State_None, runtime.state);
}

void test_math_set_clamps_sum_when_enabled() {
  V3MathRuntimeConfig cfg = {};
  cfg.inputA = 30;
  cfg.inputB = 15;
  cfg.fallbackValue = 0;
  cfg.clampMin = 0;
  cfg.clampMax = 40;
  cfg.clampEnabled = true;

  V3MathRuntimeState runtime = {};

  V3MathStepInput in = {};
  in.turnOnCondition = true;
  in.turnOffCondition = false;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_EQUAL_UINT32(40, runtime.liveValue);
}

void test_math_set_saturates_uint32_max() {
  V3MathRuntimeConfig cfg = {};
  cfg.inputA = UINT32_MAX;
  cfg.inputB = 10;
  cfg.fallbackValue = 0;
  cfg.clampEnabled = false;

  V3MathRuntimeState runtime = {};

  V3MathStepInput in = {};
  in.turnOnCondition = true;
  in.turnOffCondition = false;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, runtime.liveValue);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_math_reset_uses_fallback_and_clears_flags);
  RUN_TEST(test_math_no_set_clears_flags_keeps_current_value);
  RUN_TEST(test_math_set_computes_sum_without_clamp);
  RUN_TEST(test_math_set_clamps_sum_when_enabled);
  RUN_TEST(test_math_set_saturates_uint32_max);
  return UNITY_END();
}




