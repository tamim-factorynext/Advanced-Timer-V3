#include <unity.h>

#include "kernel/v3_condition_rules.h"
#include "../../src/kernel/v3_condition_rules.cpp"

void setUp() {}
void tearDown() {}

void test_ai_field_rejects_logical_state() {
  TEST_ASSERT_FALSE(isV3FieldAllowedForSourceType(AnalogInput, "logicalState"));
  TEST_ASSERT_TRUE(isV3FieldAllowedForSourceType(AnalogInput, "currentValue"));
}

void test_rtc_field_rejects_mission_state() {
  TEST_ASSERT_FALSE(isV3FieldAllowedForSourceType(RtcCard, "missionState"));
  TEST_ASSERT_TRUE(isV3FieldAllowedForSourceType(RtcCard, "logicalState"));
}

void test_do_and_sio_allow_mission_state_only_eq() {
  TEST_ASSERT_TRUE(isV3FieldAllowedForSourceType(DigitalOutput, "missionState"));
  TEST_ASSERT_TRUE(isV3FieldAllowedForSourceType(SoftIO, "missionState"));
  TEST_ASSERT_TRUE(isV3OperatorAllowedForField("missionState", "EQ"));
  TEST_ASSERT_FALSE(isV3OperatorAllowedForField("missionState", "NEQ"));
}

void test_bool_fields_allow_eq_neq_only() {
  TEST_ASSERT_TRUE(isV3OperatorAllowedForField("logicalState", "EQ"));
  TEST_ASSERT_TRUE(isV3OperatorAllowedForField("physicalState", "NEQ"));
  TEST_ASSERT_FALSE(isV3OperatorAllowedForField("triggerFlag", "GT"));
}

void test_card_type_token_parse() {
  logicCardType out = DigitalInput;
  TEST_ASSERT_TRUE(parseV3CardTypeToken("MATH", out));
  TEST_ASSERT_EQUAL(MathCard, out);
  TEST_ASSERT_FALSE(parseV3CardTypeToken("UNKNOWN", out));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ai_field_rejects_logical_state);
  RUN_TEST(test_rtc_field_rejects_mission_state);
  RUN_TEST(test_do_and_sio_allow_mission_state_only_eq);
  RUN_TEST(test_bool_fields_allow_eq_neq_only);
  RUN_TEST(test_card_type_token_parse);
  return UNITY_END();
}
