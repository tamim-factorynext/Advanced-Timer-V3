#include <unity.h>

#include "../../src/kernel/v3_runtime_signals.cpp"

void setUp() {}
void tearDown() {}

void test_make_runtime_signal_copies_eval_fields() {
  LogicCard card = {};
  card.type = SoftIO;
  card.state = State_DO_Active;
  card.logicalState = true;
  card.physicalState = false;
  card.triggerFlag = true;
  card.currentValue = 55;

  V3RuntimeSignal s = makeRuntimeSignal(card);
  TEST_ASSERT_EQUAL(SoftIO, s.type);
  TEST_ASSERT_EQUAL(State_DO_Active, s.state);
  TEST_ASSERT_TRUE(s.logicalState);
  TEST_ASSERT_FALSE(s.physicalState);
  TEST_ASSERT_TRUE(s.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(55, s.currentValue);
}

void test_refresh_runtime_signals_from_cards() {
  LogicCard cards[2] = {};
  cards[0].type = DigitalInput;
  cards[0].currentValue = 1;
  cards[1].type = MathCard;
  cards[1].currentValue = 9;

  V3RuntimeSignal out[2] = {};
  refreshRuntimeSignalsFromCards(cards, out, 2);

  TEST_ASSERT_EQUAL(DigitalInput, out[0].type);
  TEST_ASSERT_EQUAL_UINT32(1, out[0].currentValue);
  TEST_ASSERT_EQUAL(MathCard, out[1].type);
  TEST_ASSERT_EQUAL_UINT32(9, out[1].currentValue);
}

void test_refresh_runtime_signal_at_updates_single_slot() {
  LogicCard cards[2] = {};
  cards[0].type = DigitalInput;
  cards[0].currentValue = 1;
  cards[1].type = RtcCard;
  cards[1].currentValue = 2;

  V3RuntimeSignal out[2] = {};
  refreshRuntimeSignalsFromCards(cards, out, 2);
  cards[1].currentValue = 42;

  refreshRuntimeSignalAt(cards, out, 2, 1);

  TEST_ASSERT_EQUAL_UINT32(1, out[0].currentValue);
  TEST_ASSERT_EQUAL_UINT32(42, out[1].currentValue);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_make_runtime_signal_copies_eval_fields);
  RUN_TEST(test_refresh_runtime_signals_from_cards);
  RUN_TEST(test_refresh_runtime_signal_at_updates_single_slot);
  return UNITY_END();
}

