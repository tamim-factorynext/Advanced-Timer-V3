#include <unity.h>

#include <string>

#include "../../src/kernel/v3_typed_config_rules.cpp"

namespace {
constexpr uint8_t kDoStart = 4;
constexpr uint8_t kAiStart = 8;
constexpr uint8_t kSioStart = 10;
constexpr uint8_t kMathStart = 14;
constexpr uint8_t kRtcStart = 16;
constexpr uint8_t kTotalCards = 18;

V3CardFamily familyForId(uint8_t id) {
  if (id < kDoStart) return V3CardFamily::DI;
  if (id < kAiStart) return V3CardFamily::DO;
  if (id < kSioStart) return V3CardFamily::AI;
  if (id < kMathStart) return V3CardFamily::SIO;
  if (id < kRtcStart) return V3CardFamily::MATH;
  return V3CardFamily::RTC;
}

void setDefaultCondition(V3ConditionBlock& block) {
  block.clauseAId = 0;
  block.clauseAOperator = Op_AlwaysFalse;
  block.clauseAThreshold = 0;
  block.clauseBId = 0;
  block.clauseBOperator = Op_AlwaysFalse;
  block.clauseBThreshold = 0;
  block.combiner = Combine_None;
}

void seedValidCards(V3CardConfig* cards) {
  for (uint8_t i = 0; i < kTotalCards; ++i) {
    cards[i] = {};
    cards[i].cardId = i;
    cards[i].family = familyForId(i);
    cards[i].enabled = true;
    cards[i].faultPolicy = V3FaultPolicy::WARN;

    setDefaultCondition(cards[i].di.set);
    setDefaultCondition(cards[i].di.reset);
    setDefaultCondition(cards[i].dout.set);
    setDefaultCondition(cards[i].dout.reset);
    setDefaultCondition(cards[i].sio.set);
    setDefaultCondition(cards[i].sio.reset);
    setDefaultCondition(cards[i].math.set);
    setDefaultCondition(cards[i].math.reset);

    cards[i].di.edgeMode = Mode_DI_Rising;
    cards[i].dout.mode = Mode_DO_Normal;
    cards[i].sio.mode = Mode_DO_Normal;
    cards[i].ai.inputMin = 0;
    cards[i].ai.inputMax = 4095;
    cards[i].ai.outputMin = 0;
    cards[i].ai.outputMax = 10000;
    cards[i].ai.emaAlphaX100 = 50;
    cards[i].rtc.hour = 12;
    cards[i].rtc.minute = 30;
  }
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_validate_typed_cards_accepts_valid_layout() {
  V3CardConfig cards[kTotalCards] = {};
  seedValidCards(cards);

  std::string reason;
  bool ok = validateTypedCardConfigs(cards, kTotalCards, kDoStart, kAiStart,
                                     kSioStart, kMathStart, kRtcStart, reason);
  TEST_ASSERT_TRUE(ok);
}

void test_validate_typed_cards_rejects_family_slot_mismatch() {
  V3CardConfig cards[kTotalCards] = {};
  seedValidCards(cards);
  cards[8].family = V3CardFamily::DO;  // id=8 is AI slot.

  std::string reason;
  bool ok = validateTypedCardConfigs(cards, kTotalCards, kDoStart, kAiStart,
                                     kSioStart, kMathStart, kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_validate_typed_cards_rejects_do_mode_mismatch() {
  V3CardConfig cards[kTotalCards] = {};
  seedValidCards(cards);
  cards[4].dout.mode = Mode_AI_Continuous;

  std::string reason;
  bool ok = validateTypedCardConfigs(cards, kTotalCards, kDoStart, kAiStart,
                                     kSioStart, kMathStart, kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_validate_typed_cards_rejects_illegal_operator_for_ai_source() {
  V3CardConfig cards[kTotalCards] = {};
  seedValidCards(cards);
  cards[4].dout.set.clauseAId = 8;  // AI source
  cards[4].dout.set.clauseAOperator = Op_LogicalTrue;
  cards[4].dout.set.combiner = Combine_None;

  std::string reason;
  bool ok = validateTypedCardConfigs(cards, kTotalCards, kDoStart, kAiStart,
                                     kSioStart, kMathStart, kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_validate_typed_cards_rejects_rtc_minute_out_of_range() {
  V3CardConfig cards[kTotalCards] = {};
  seedValidCards(cards);
  cards[16].rtc.minute = 77;

  std::string reason;
  bool ok = validateTypedCardConfigs(cards, kTotalCards, kDoStart, kAiStart,
                                     kSioStart, kMathStart, kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_validate_typed_cards_accepts_valid_layout);
  RUN_TEST(test_validate_typed_cards_rejects_family_slot_mismatch);
  RUN_TEST(test_validate_typed_cards_rejects_do_mode_mismatch);
  RUN_TEST(test_validate_typed_cards_rejects_illegal_operator_for_ai_source);
  RUN_TEST(test_validate_typed_cards_rejects_rtc_minute_out_of_range);
  return UNITY_END();
}
