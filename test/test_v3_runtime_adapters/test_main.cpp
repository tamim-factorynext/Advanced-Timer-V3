#include <unity.h>

#include "../../src/kernel/v3_runtime_adapters.cpp"

void setUp() {}
void tearDown() {}

void test_ai_adapter_maps_legacy_fields_to_typed_config() {
  LogicCard card = {};
  card.setting1 = 10;
  card.setting2 = 20;
  card.startOnMs = 30;
  card.startOffMs = 40;
  card.setting3 = 500;

  V3AiRuntimeConfig cfg = makeAiRuntimeConfig(card);
  TEST_ASSERT_EQUAL_UINT32(10, cfg.inputMin);
  TEST_ASSERT_EQUAL_UINT32(20, cfg.inputMax);
  TEST_ASSERT_EQUAL_UINT32(30, cfg.outputMin);
  TEST_ASSERT_EQUAL_UINT32(40, cfg.outputMax);
  TEST_ASSERT_EQUAL_UINT32(500, cfg.emaAlphaX1000);
}

void test_do_adapter_roundtrip_runtime_state() {
  LogicCard card = {};
  card.logicalState = true;
  card.physicalState = false;
  card.triggerFlag = true;
  card.currentValue = 9;
  card.startOnMs = 11;
  card.startOffMs = 22;
  card.repeatCounter = 3;
  card.state = State_DO_Active;

  V3DoRuntimeState runtime = makeDoRuntimeState(card);
  runtime.physicalState = true;
  runtime.currentValue = 10;
  runtime.repeatCounter = 4;
  runtime.state = State_DO_Finished;

  applyDoRuntimeState(card, runtime);
  TEST_ASSERT_TRUE(card.logicalState);
  TEST_ASSERT_TRUE(card.physicalState);
  TEST_ASSERT_TRUE(card.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(10, card.currentValue);
  TEST_ASSERT_EQUAL_UINT32(11, card.startOnMs);
  TEST_ASSERT_EQUAL_UINT32(22, card.startOffMs);
  TEST_ASSERT_EQUAL_UINT32(4, card.repeatCounter);
  TEST_ASSERT_EQUAL(State_DO_Finished, card.state);
}

void test_rtc_adapter_maps_trigger_start_and_duration() {
  LogicCard card = {};
  card.setting1 = 60000;
  card.logicalState = true;
  card.startOnMs = 12345;

  V3RtcRuntimeConfig cfg = makeRtcRuntimeConfig(card);
  V3RtcRuntimeState runtime = makeRtcRuntimeState(card);

  TEST_ASSERT_EQUAL_UINT32(60000, cfg.triggerDurationMs);
  TEST_ASSERT_TRUE(runtime.logicalState);
  TEST_ASSERT_EQUAL_UINT32(12345, runtime.triggerStartMs);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ai_adapter_maps_legacy_fields_to_typed_config);
  RUN_TEST(test_do_adapter_roundtrip_runtime_state);
  RUN_TEST(test_rtc_adapter_maps_trigger_start_and_duration);
  return UNITY_END();
}

