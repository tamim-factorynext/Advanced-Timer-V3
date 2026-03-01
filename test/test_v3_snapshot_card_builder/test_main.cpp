#include <unity.h>

#include "../../src/kernel/v3_runtime_adapters.cpp"
#include "../../src/kernel/v3_runtime_store.cpp"
#include "../../src/runtime/snapshot_card_builder.cpp"

void setUp() {}
void tearDown() {}

void test_build_runtime_snapshot_card_for_do_uses_typed_runtime_state() {
  RuntimeCardMeta meta = {};
  meta.id = 4;
  meta.type = DigitalOutput;
  meta.index = 0;
  meta.mode = Mode_DO_Gated;

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
  dOut[0].state = State_DO_Active;
  dOut[0].currentValue = 1;
  dOut[0].startOnMs = 100;
  dOut[0].startOffMs = 120;
  dOut[0].repeatCounter = 3;

  RuntimeSnapshotCard snap = buildRuntimeSnapshotCard(meta, store);
  TEST_ASSERT_EQUAL_UINT8(4, snap.id);
  TEST_ASSERT_EQUAL(DigitalOutput, snap.type);
  TEST_ASSERT_EQUAL(Mode_DO_Gated, snap.mode);
  TEST_ASSERT_TRUE(snap.logicalState);
  TEST_ASSERT_TRUE(snap.physicalState);
  TEST_ASSERT_TRUE(snap.triggerFlag);
  TEST_ASSERT_EQUAL(State_DO_Active, snap.state);
  TEST_ASSERT_EQUAL_UINT32(1, snap.currentValue);
  TEST_ASSERT_EQUAL_UINT32(100, snap.startOnMs);
  TEST_ASSERT_EQUAL_UINT32(120, snap.startOffMs);
  TEST_ASSERT_EQUAL_UINT32(3, snap.repeatCounter);
}

void test_build_runtime_snapshot_cards_for_mixed_families() {
  RuntimeCardMeta cards[2] = {};
  cards[0].id = 8;
  cards[0].type = AnalogInput;
  cards[0].index = 0;
  cards[0].mode = Mode_AI_Continuous;
  cards[1].id = 12;
  cards[1].type = RtcCard;
  cards[1].index = 0;

  V3DiRuntimeState di[1] = {};
  V3DoRuntimeState dOut[1] = {};
  V3AiRuntimeState ai[1] = {};
  V3SioRuntimeState sio[1] = {};
  V3MathRuntimeState math[1] = {};
  V3RtcRuntimeState rtc[1] = {};
  V3RuntimeStoreView store = {di, 1, dOut, 1, ai, 1, sio, 1, math, 1, rtc, 1};

  ai[0].currentValue = 2345;
  ai[0].mode = Mode_AI_Continuous;
  ai[0].state = State_AI_Streaming;
  rtc[0].logicalState = true;
  rtc[0].triggerFlag = true;
  rtc[0].currentValue = 1;
  rtc[0].triggerStartMs = 777;
  rtc[0].mode = Mode_None;
  rtc[0].state = State_None;

  RuntimeSnapshotCard out[2] = {};
  buildRuntimeSnapshotCards(cards, 2, store, out);

  TEST_ASSERT_EQUAL_UINT8(8, out[0].id);
  TEST_ASSERT_EQUAL(AnalogInput, out[0].type);
  TEST_ASSERT_EQUAL_UINT32(2345, out[0].currentValue);
  TEST_ASSERT_EQUAL(State_AI_Streaming, out[0].state);
  TEST_ASSERT_EQUAL(Mode_AI_Continuous, out[0].mode);

  TEST_ASSERT_EQUAL_UINT8(12, out[1].id);
  TEST_ASSERT_EQUAL(RtcCard, out[1].type);
  TEST_ASSERT_TRUE(out[1].logicalState);
  TEST_ASSERT_TRUE(out[1].triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(1, out[1].currentValue);
  TEST_ASSERT_EQUAL_UINT32(777, out[1].startOnMs);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_build_runtime_snapshot_card_for_do_uses_typed_runtime_state);
  RUN_TEST(test_build_runtime_snapshot_cards_for_mixed_families);
  return UNITY_END();
}
