#include <unity.h>

#include "../../src/kernel/v3_config_sanitize.cpp"

void setUp() {}
void tearDown() {}

void test_do_runtime_fields_are_cleared_from_config_card() {
  LogicCard card = {};
  card.type = DigitalOutput;
  card.startOnMs = 777;
  card.startOffMs = 888;
  card.logicalState = true;
  card.physicalState = true;
  card.triggerFlag = true;
  card.currentValue = 42;
  card.repeatCounter = 5;
  card.state = State_DO_Active;

  sanitizeConfigCardRuntimeFields(card);

  TEST_ASSERT_FALSE(card.logicalState);
  TEST_ASSERT_FALSE(card.physicalState);
  TEST_ASSERT_FALSE(card.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(0, card.currentValue);
  TEST_ASSERT_EQUAL_UINT32(0, card.repeatCounter);
  TEST_ASSERT_EQUAL_UINT32(0, card.startOnMs);
  TEST_ASSERT_EQUAL_UINT32(0, card.startOffMs);
  TEST_ASSERT_EQUAL(State_DO_Idle, card.state);
}

void test_ai_config_ranges_are_preserved_while_runtime_clears() {
  LogicCard card = {};
  card.type = AnalogInput;
  card.startOnMs = 123;   // config output min
  card.startOffMs = 987;  // config output max
  card.logicalState = true;
  card.currentValue = 555;
  card.repeatCounter = 2;
  card.state = State_DO_Active;

  sanitizeConfigCardRuntimeFields(card);

  TEST_ASSERT_EQUAL_UINT32(123, card.startOnMs);
  TEST_ASSERT_EQUAL_UINT32(987, card.startOffMs);
  TEST_ASSERT_FALSE(card.logicalState);
  TEST_ASSERT_EQUAL_UINT32(0, card.currentValue);
  TEST_ASSERT_EQUAL_UINT32(0, card.repeatCounter);
  TEST_ASSERT_EQUAL(State_AI_Streaming, card.state);
}

void test_rtc_runtime_start_is_cleared_but_duration_setting_remains() {
  LogicCard card = {};
  card.type = RtcCard;
  card.setting1 = 60000;  // trigger duration config
  card.startOnMs = 12345; // runtime trigger start
  card.triggerFlag = true;
  card.state = State_DO_Active;

  sanitizeConfigCardRuntimeFields(card);

  TEST_ASSERT_EQUAL_UINT32(60000, card.setting1);
  TEST_ASSERT_EQUAL_UINT32(0, card.startOnMs);
  TEST_ASSERT_FALSE(card.triggerFlag);
  TEST_ASSERT_EQUAL(State_None, card.state);
}

void test_array_sanitize_applies_to_all_cards() {
  LogicCard cards[2] = {};
  cards[0].type = DigitalInput;
  cards[0].currentValue = 1;
  cards[0].state = State_DO_Active;
  cards[1].type = MathCard;
  cards[1].currentValue = 9;
  cards[1].state = State_DO_Active;

  sanitizeConfigCardsRuntimeFields(cards, 2);

  TEST_ASSERT_EQUAL_UINT32(0, cards[0].currentValue);
  TEST_ASSERT_EQUAL(State_DI_Idle, cards[0].state);
  TEST_ASSERT_EQUAL_UINT32(0, cards[1].currentValue);
  TEST_ASSERT_EQUAL(State_None, cards[1].state);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_do_runtime_fields_are_cleared_from_config_card);
  RUN_TEST(test_ai_config_ranges_are_preserved_while_runtime_clears);
  RUN_TEST(test_rtc_runtime_start_is_cleared_but_duration_setting_remains);
  RUN_TEST(test_array_sanitize_applies_to_all_cards);
  return UNITY_END();
}
