#include <unity.h>

#include "../../src/kernel/v3_status_runtime.cpp"

void setUp() {}
void tearDown() {}

void test_do_mission_states() {
  TEST_ASSERT_TRUE(isMissionRunning(DigitalOutput, State_DO_OnDelay));
  TEST_ASSERT_TRUE(isMissionRunning(DigitalOutput, State_DO_Active));
  TEST_ASSERT_TRUE(isMissionFinished(DigitalOutput, State_DO_Finished));
  TEST_ASSERT_TRUE(isMissionStopped(DigitalOutput, State_DO_Idle));
}

void test_sio_mission_states() {
  TEST_ASSERT_TRUE(isMissionRunning(SoftIO, State_DO_Active));
  TEST_ASSERT_TRUE(isMissionFinished(SoftIO, State_DO_Finished));
  TEST_ASSERT_TRUE(isMissionStopped(SoftIO, State_DO_Finished));
}

void test_non_mission_types_reject_mission_state_ops() {
  TEST_ASSERT_FALSE(isMissionRunning(DigitalInput, State_DO_Active));
  TEST_ASSERT_FALSE(isMissionFinished(AnalogInput, State_DO_Finished));
  TEST_ASSERT_FALSE(isMissionStopped(MathCard, State_DO_Idle));
  TEST_ASSERT_FALSE(isMissionStopped(RtcCard, State_DO_Finished));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_do_mission_states);
  RUN_TEST(test_sio_mission_states);
  RUN_TEST(test_non_mission_types_reject_mission_state_ops);
  return UNITY_END();
}

