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
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 500;
  runtime.state = State_DO_Active;

  V3MathStepInput in = {};
  in.setCondition = true;
  in.resetCondition = true;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(out.setResult);
  TEST_ASSERT_TRUE(out.resetResult);
  TEST_ASSERT_TRUE(out.resetOverride);
  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(77, runtime.currentValue);
  TEST_ASSERT_EQUAL(State_None, runtime.state);
}

void test_math_no_set_clears_flags_keeps_current_value() {
  V3MathRuntimeConfig cfg = {};
  cfg.inputA = 1;
  cfg.inputB = 2;
  cfg.fallbackValue = 9;

  V3MathRuntimeState runtime = {};
  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;
  runtime.currentValue = 1234;
  runtime.state = State_DO_Active;

  V3MathStepInput in = {};
  in.setCondition = false;
  in.resetCondition = false;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_FALSE(out.setResult);
  TEST_ASSERT_FALSE(out.resetResult);
  TEST_ASSERT_FALSE(out.resetOverride);
  TEST_ASSERT_FALSE(runtime.logicalState);
  TEST_ASSERT_FALSE(runtime.physicalState);
  TEST_ASSERT_FALSE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(1234, runtime.currentValue);
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
  in.setCondition = true;
  in.resetCondition = false;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_TRUE(runtime.logicalState);
  TEST_ASSERT_TRUE(runtime.physicalState);
  TEST_ASSERT_TRUE(runtime.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(18, runtime.currentValue);
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
  in.setCondition = true;
  in.resetCondition = false;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_EQUAL_UINT32(40, runtime.currentValue);
}

void test_math_set_saturates_uint32_max() {
  V3MathRuntimeConfig cfg = {};
  cfg.inputA = UINT32_MAX;
  cfg.inputB = 10;
  cfg.fallbackValue = 0;
  cfg.clampEnabled = false;

  V3MathRuntimeState runtime = {};

  V3MathStepInput in = {};
  in.setCondition = true;
  in.resetCondition = false;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, runtime, in, out);

  TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, runtime.currentValue);
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

