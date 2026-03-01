#include <unity.h>

#include "../../src/kernel/v3_ai_runtime.cpp"

void setUp() {}
void tearDown() {}

void test_ai_alpha_1000_tracks_scaled_value() {
  V3AiRuntimeConfig cfg = {};
  cfg.inputMin = 0;
  cfg.inputMax = 100;
  cfg.outputMin = 0;
  cfg.outputMax = 1000;
  cfg.emaAlphaX1000 = 1000;

  V3AiRuntimeState runtime = {};
  runtime.currentValue = 0;

  V3AiStepInput in = {};
  in.rawSample = 50;

  runV3AiStep(cfg, runtime, in);

  TEST_ASSERT_EQUAL_UINT32(500, runtime.currentValue);
  TEST_ASSERT_EQUAL(Mode_AI_Continuous, runtime.mode);
  TEST_ASSERT_EQUAL(State_AI_Streaming, runtime.state);
}

void test_ai_alpha_0_keeps_previous_value() {
  V3AiRuntimeConfig cfg = {};
  cfg.inputMin = 0;
  cfg.inputMax = 100;
  cfg.outputMin = 0;
  cfg.outputMax = 1000;
  cfg.emaAlphaX1000 = 0;

  V3AiRuntimeState runtime = {};
  runtime.currentValue = 333;

  V3AiStepInput in = {};
  in.rawSample = 90;

  runV3AiStep(cfg, runtime, in);

  TEST_ASSERT_EQUAL_UINT32(333, runtime.currentValue);
}

void test_ai_input_bounds_are_order_independent() {
  V3AiRuntimeConfig cfg = {};
  cfg.inputMin = 100;
  cfg.inputMax = 0;
  cfg.outputMin = 0;
  cfg.outputMax = 1000;
  cfg.emaAlphaX1000 = 1000;

  V3AiRuntimeState runtime = {};
  runtime.currentValue = 0;

  V3AiStepInput in = {};
  in.rawSample = 25;

  runV3AiStep(cfg, runtime, in);

  TEST_ASSERT_EQUAL_UINT32(250, runtime.currentValue);
}

void test_ai_equal_input_range_uses_output_min() {
  V3AiRuntimeConfig cfg = {};
  cfg.inputMin = 20;
  cfg.inputMax = 20;
  cfg.outputMin = 123;
  cfg.outputMax = 999;
  cfg.emaAlphaX1000 = 1000;

  V3AiRuntimeState runtime = {};
  runtime.currentValue = 0;

  V3AiStepInput in = {};
  in.rawSample = 9999;

  runV3AiStep(cfg, runtime, in);

  TEST_ASSERT_EQUAL_UINT32(123, runtime.currentValue);
}

void test_ai_alpha_is_clamped_to_1000() {
  V3AiRuntimeConfig cfg = {};
  cfg.inputMin = 0;
  cfg.inputMax = 100;
  cfg.outputMin = 0;
  cfg.outputMax = 1000;
  cfg.emaAlphaX1000 = 1500;

  V3AiRuntimeState runtime = {};
  runtime.currentValue = 0;

  V3AiStepInput in = {};
  in.rawSample = 80;

  runV3AiStep(cfg, runtime, in);

  TEST_ASSERT_EQUAL_UINT32(800, runtime.currentValue);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ai_alpha_1000_tracks_scaled_value);
  RUN_TEST(test_ai_alpha_0_keeps_previous_value);
  RUN_TEST(test_ai_input_bounds_are_order_independent);
  RUN_TEST(test_ai_equal_input_range_uses_output_min);
  RUN_TEST(test_ai_alpha_is_clamped_to_1000);
  return UNITY_END();
}

