#include <unity.h>

#include "../../src/kernel/v3_runtime_signals.cpp"

void setUp() {}
void tearDown() {}

void test_make_runtime_signal_reads_store_from_meta() {
  RuntimeCardMeta meta = {};
  meta.type = SoftIO;
  meta.index = 0;

  V3SioRuntimeState sio[1] = {};
  sio[0].state = State_DO_Active;
  sio[0].logicalState = true;
  sio[0].physicalState = false;
  sio[0].triggerFlag = true;
  sio[0].currentValue = 55;

  V3RuntimeStoreView store = {};
  store.sio = sio;
  store.sioCount = 1;

  V3RuntimeSignal s = makeRuntimeSignal(meta, store);
  TEST_ASSERT_EQUAL(SoftIO, s.type);
  TEST_ASSERT_EQUAL(State_DO_Active, s.state);
  TEST_ASSERT_TRUE(s.logicalState);
  TEST_ASSERT_FALSE(s.physicalState);
  TEST_ASSERT_TRUE(s.triggerFlag);
  TEST_ASSERT_EQUAL_UINT32(55, s.currentValue);
}

void test_refresh_runtime_signals_from_runtime() {
  RuntimeCardMeta cards[2] = {};
  cards[0].type = DigitalInput;
  cards[0].index = 0;
  cards[1].type = MathCard;
  cards[1].index = 0;

  V3DiRuntimeState di[1] = {};
  di[0].currentValue = 1;
  V3MathRuntimeState math[1] = {};
  math[0].currentValue = 9;

  V3RuntimeStoreView store = {};
  store.di = di;
  store.diCount = 1;
  store.math = math;
  store.mathCount = 1;

  V3RuntimeSignal out[2] = {};
  refreshRuntimeSignalsFromRuntime(cards, store, out, 2);

  TEST_ASSERT_EQUAL(DigitalInput, out[0].type);
  TEST_ASSERT_EQUAL_UINT32(1, out[0].currentValue);
  TEST_ASSERT_EQUAL(MathCard, out[1].type);
  TEST_ASSERT_EQUAL_UINT32(9, out[1].currentValue);
}

void test_refresh_runtime_signal_at_updates_single_slot() {
  RuntimeCardMeta cards[2] = {};
  cards[0].type = DigitalInput;
  cards[0].index = 0;
  cards[1].type = RtcCard;
  cards[1].index = 0;

  V3DiRuntimeState di[1] = {};
  di[0].currentValue = 1;
  V3RtcRuntimeState rtc[1] = {};
  rtc[0].currentValue = 2;

  V3RuntimeStoreView store = {};
  store.di = di;
  store.diCount = 1;
  store.rtc = rtc;
  store.rtcCount = 1;

  V3RuntimeSignal out[2] = {};
  refreshRuntimeSignalsFromRuntime(cards, store, out, 2);
  rtc[0].currentValue = 42;

  refreshRuntimeSignalAt(cards, store, out, 2, 1);

  TEST_ASSERT_EQUAL_UINT32(1, out[0].currentValue);
  TEST_ASSERT_EQUAL_UINT32(42, out[1].currentValue);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_make_runtime_signal_reads_store_from_meta);
  RUN_TEST(test_refresh_runtime_signals_from_runtime);
  RUN_TEST(test_refresh_runtime_signal_at_updates_single_slot);
  return UNITY_END();
}
