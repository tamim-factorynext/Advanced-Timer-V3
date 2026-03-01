#include <unity.h>

#include "../../src/kernel/v3_runtime_adapters.cpp"
#include "../../src/kernel/v3_runtime_store.cpp"

void setUp() {}
void tearDown() {}

void test_sync_runtime_store_from_typed_cards_copies_family_state() {
  LogicCard cards[3] = {};
  cards[0].index = 0;
  cards[0].logicalState = true;
  cards[1].index = 0;
  cards[1].currentValue = 321;
  cards[1].state = State_AI_Streaming;
  cards[2].index = 0;
  cards[2].triggerFlag = true;
  cards[2].startOnMs = 7788;

  V3CardConfig typed[3] = {};
  typed[0].family = V3CardFamily::DI;
  typed[0].di.channel = 0;
  typed[1].family = V3CardFamily::AI;
  typed[1].ai.channel = 0;
  typed[2].family = V3CardFamily::RTC;

  V3DiRuntimeState di[1] = {};
  V3DoRuntimeState dOut[1] = {};
  V3AiRuntimeState ai[1] = {};
  V3SioRuntimeState sio[1] = {};
  V3MathRuntimeState math[1] = {};
  V3RtcRuntimeState rtc[1] = {};
  V3RuntimeStoreView store = {di, 1, dOut, 1, ai, 1, sio, 1, math, 1, rtc, 1};

  syncRuntimeStoreFromTypedCards(cards, typed, 3, store);

  TEST_ASSERT_TRUE(di[0].logicalState);
  TEST_ASSERT_EQUAL_UINT32(321, ai[0].currentValue);
  TEST_ASSERT_EQUAL(State_AI_Streaming, ai[0].state);
  TEST_ASSERT_TRUE(rtc[0].triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(7788, rtc[0].triggerStartMs);
}

void test_mirror_runtime_store_card_to_legacy_by_typed_updates_card() {
  LogicCard card = {};
  card.index = 0;
  V3CardConfig typed = {};
  typed.family = V3CardFamily::DO;
  typed.dout.channel = 0;

  V3DiRuntimeState di[1] = {};
  V3DoRuntimeState dOut[1] = {};
  V3AiRuntimeState ai[1] = {};
  V3SioRuntimeState sio[1] = {};
  V3MathRuntimeState math[1] = {};
  V3RtcRuntimeState rtc[1] = {};
  V3RuntimeStoreView store = {di, 1, dOut, 1, ai, 1, sio, 1, math, 1, rtc, 1};

  dOut[0].logicalState = true;
  dOut[0].physicalState = true;
  dOut[0].triggerFlag = true;
  dOut[0].currentValue = 5;
  dOut[0].startOnMs = 100;
  dOut[0].startOffMs = 200;
  dOut[0].repeatCounter = 2;
  dOut[0].state = State_DO_Active;

  mirrorRuntimeStoreCardToLegacyByTyped(card, typed, store);

  TEST_ASSERT_TRUE(card.logicalState);
  TEST_ASSERT_TRUE(card.physicalState);
  TEST_ASSERT_TRUE(card.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(5, card.currentValue);
  TEST_ASSERT_EQUAL_UINT32(100, card.startOnMs);
  TEST_ASSERT_EQUAL_UINT32(200, card.startOffMs);
  TEST_ASSERT_EQUAL_UINT32(2, card.repeatCounter);
  TEST_ASSERT_EQUAL(State_DO_Active, card.state);
}

void test_runtime_state_lookup_rejects_out_of_range() {
  V3DiRuntimeState di[2] = {};
  V3DoRuntimeState dOut[1] = {};
  V3AiRuntimeState ai[1] = {};
  V3SioRuntimeState sio[1] = {};
  V3MathRuntimeState math[1] = {};
  V3RtcRuntimeState rtc[1] = {};
  V3RuntimeStoreView store = {di, 2, dOut, 1, ai, 1, sio, 1, math, 1, rtc, 1};

  TEST_ASSERT_NULL(runtimeDiStateAt(2, store));
  TEST_ASSERT_NULL(runtimeDoStateAt(1, store));
  TEST_ASSERT_NULL(runtimeAiStateAt(1, store));
  TEST_ASSERT_NULL(runtimeSioStateAt(1, store));
  TEST_ASSERT_NULL(runtimeMathStateAt(1, store));
  TEST_ASSERT_NULL(runtimeRtcStateAt(1, store));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sync_runtime_store_from_typed_cards_copies_family_state);
  RUN_TEST(test_mirror_runtime_store_card_to_legacy_by_typed_updates_card);
  RUN_TEST(test_runtime_state_lookup_rejects_out_of_range);
  return UNITY_END();
}
