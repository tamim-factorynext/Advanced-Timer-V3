#include <unity.h>

#include <ArduinoJson.h>

#include "../../src/kernel/v3_condition_rules.cpp"
#include "../../src/kernel/v3_payload_rules.cpp"

void setUp() {}
void tearDown() {}

constexpr uint8_t kDoStart = 4;
constexpr uint8_t kAiStart = 8;
constexpr uint8_t kSioStart = 10;
constexpr uint8_t kMathStart = 14;
constexpr uint8_t kRtcStart = 16;
constexpr uint8_t kTotalCards = 18;

void addBareCards(JsonArray& cards) {
  for (uint8_t id = 0; id < kTotalCards; ++id) {
    JsonObject c = cards.add<JsonObject>();
    c["cardId"] = id;
    if (id < kDoStart) {
      c["cardType"] = "DI";
    } else if (id < kAiStart) {
      c["cardType"] = "DO";
    } else if (id < kSioStart) {
      c["cardType"] = "AI";
    } else if (id < kMathStart) {
      c["cardType"] = "SIO";
    } else if (id < kRtcStart) {
      c["cardType"] = "MATH";
    } else {
      c["cardType"] = "RTC";
    }
    c["config"].to<JsonObject>();
  }
}

void writeBlockNumeric(JsonObject cfg, uint8_t sourceId, const char* field,
                       const char* op, uint32_t threshold) {
  JsonObject set = cfg["set"].to<JsonObject>();
  set["combiner"] = "NONE";
  JsonObject clauseA = set["clauseA"].to<JsonObject>();
  JsonObject source = clauseA["source"].to<JsonObject>();
  source["cardId"] = sourceId;
  source["field"] = field;
  source["type"] = "BOOL";
  clauseA["operator"] = op;
  clauseA["threshold"] = threshold;
}

void writeBlockState(JsonObject cfg, uint8_t sourceId, const char* field,
                     const char* op, const char* threshold) {
  JsonObject set = cfg["set"].to<JsonObject>();
  set["combiner"] = "NONE";
  JsonObject clauseA = set["clauseA"].to<JsonObject>();
  JsonObject source = clauseA["source"].to<JsonObject>();
  source["cardId"] = sourceId;
  source["field"] = field;
  source["type"] = "STATE";
  clauseA["operator"] = op;
  clauseA["threshold"] = threshold;
}

void test_payload_rejects_ai_logical_state() {
  JsonDocument doc;
  JsonArray cards = doc.to<JsonArray>();
  addBareCards(cards);
  JsonObject cfg = cards[0]["config"].as<JsonObject>();
  writeBlockNumeric(cfg, 8, "logicalState", "EQ", 1);

  std::string reason;
  bool ok = validateV3PayloadConditionSources(cards, kTotalCards, kDoStart,
                                              kAiStart, kSioStart, kMathStart,
                                              kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_payload_rejects_rtc_mission_state() {
  JsonDocument doc;
  JsonArray cards = doc.to<JsonArray>();
  addBareCards(cards);
  JsonObject cfg = cards[4]["config"].as<JsonObject>();
  writeBlockState(cfg, 16, "missionState", "EQ", "ACTIVE");

  std::string reason;
  bool ok = validateV3PayloadConditionSources(cards, kTotalCards, kDoStart,
                                              kAiStart, kSioStart, kMathStart,
                                              kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_payload_rejects_mission_state_neq() {
  JsonDocument doc;
  JsonArray cards = doc.to<JsonArray>();
  addBareCards(cards);
  JsonObject cfg = cards[4]["config"].as<JsonObject>();
  writeBlockState(cfg, 4, "missionState", "NEQ", "ACTIVE");

  std::string reason;
  bool ok = validateV3PayloadConditionSources(cards, kTotalCards, kDoStart,
                                              kAiStart, kSioStart, kMathStart,
                                              kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_payload_accepts_do_mission_state_eq() {
  JsonDocument doc;
  JsonArray cards = doc.to<JsonArray>();
  addBareCards(cards);
  JsonObject cfg = cards[4]["config"].as<JsonObject>();
  writeBlockState(cfg, 4, "missionState", "EQ", "ACTIVE");

  std::string reason;
  bool ok = validateV3PayloadConditionSources(cards, kTotalCards, kDoStart,
                                              kAiStart, kSioStart, kMathStart,
                                              kRtcStart, reason);
  TEST_ASSERT_TRUE(ok);
}

void test_payload_rejects_duplicate_card_id() {
  JsonDocument doc;
  JsonArray cards = doc.to<JsonArray>();
  addBareCards(cards);
  cards[5]["cardId"] = 4;

  std::string reason;
  bool ok = validateV3PayloadConditionSources(cards, kTotalCards, kDoStart,
                                              kAiStart, kSioStart, kMathStart,
                                              kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_payload_rejects_card_type_slot_mismatch() {
  JsonDocument doc;
  JsonArray cards = doc.to<JsonArray>();
  addBareCards(cards);
  cards[8]["cardType"] = "DO";  // id=8 is AI slot in this profile layout.

  std::string reason;
  bool ok = validateV3PayloadConditionSources(cards, kTotalCards, kDoStart,
                                              kAiStart, kSioStart, kMathStart,
                                              kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_payload_rejects_clause_source_card_id_out_of_range() {
  JsonDocument doc;
  JsonArray cards = doc.to<JsonArray>();
  addBareCards(cards);

  JsonObject cfg = cards[4]["config"].as<JsonObject>();
  JsonObject set = cfg["set"].to<JsonObject>();
  set["combiner"] = "NONE";
  JsonObject clauseA = set["clauseA"].to<JsonObject>();
  JsonObject source = clauseA["source"].to<JsonObject>();
  source["cardId"] = 250;
  source["field"] = "currentValue";
  source["type"] = "NUMBER";
  clauseA["operator"] = "GT";
  clauseA["threshold"] = 1;

  std::string reason;
  bool ok = validateV3PayloadConditionSources(cards, kTotalCards, kDoStart,
                                              kAiStart, kSioStart, kMathStart,
                                              kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_payload_rejects_invalid_mission_state_threshold() {
  JsonDocument doc;
  JsonArray cards = doc.to<JsonArray>();
  addBareCards(cards);
  JsonObject cfg = cards[4]["config"].as<JsonObject>();
  writeBlockState(cfg, 4, "missionState", "EQ", "PAUSED");

  std::string reason;
  bool ok = validateV3PayloadConditionSources(cards, kTotalCards, kDoStart,
                                              kAiStart, kSioStart, kMathStart,
                                              kRtcStart, reason);
  TEST_ASSERT_FALSE(ok);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_payload_rejects_ai_logical_state);
  RUN_TEST(test_payload_rejects_rtc_mission_state);
  RUN_TEST(test_payload_rejects_mission_state_neq);
  RUN_TEST(test_payload_accepts_do_mission_state_eq);
  RUN_TEST(test_payload_rejects_duplicate_card_id);
  RUN_TEST(test_payload_rejects_card_type_slot_mismatch);
  RUN_TEST(test_payload_rejects_clause_source_card_id_out_of_range);
  RUN_TEST(test_payload_rejects_invalid_mission_state_threshold);
  return UNITY_END();
}
