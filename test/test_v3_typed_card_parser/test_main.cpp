#include <unity.h>

#include "../../src/kernel/v3_condition_rules.cpp"
#include "../../src/kernel/v3_typed_card_parser.cpp"

namespace {
constexpr uint8_t kDoStart = 4;
constexpr uint8_t kAiStart = 8;
constexpr uint8_t kSioStart = 10;
constexpr uint8_t kMathStart = 14;
constexpr uint8_t kRtcStart = 16;
constexpr uint8_t kTotalCards = 18;

logicCardType expectedType(uint8_t id) {
  if (id < kDoStart) return DigitalInput;
  if (id < kAiStart) return DigitalOutput;
  if (id < kSioStart) return AnalogInput;
  if (id < kMathStart) return SoftIO;
  if (id < kRtcStart) return MathCard;
  return RtcCard;
}

void seedSourceMap(logicCardType* out) {
  for (uint8_t i = 0; i < kTotalCards; ++i) {
    out[i] = expectedType(i);
  }
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_parse_di_card_success() {
  logicCardType sourceMap[kTotalCards] = {};
  seedSourceMap(sourceMap);

  JsonDocument doc;
  JsonObject card = doc.to<JsonObject>();
  card["cardId"] = 0;
  card["cardType"] = "DI";
  card["enabled"] = true;
  card["faultPolicy"] = "WARN";
  JsonObject cfg = card["config"].to<JsonObject>();
  cfg["channel"] = 0;
  cfg["invert"] = false;
  cfg["debounceTime"] = 10;
  cfg["edgeMode"] = "RISING";
  JsonObject set = cfg["set"].to<JsonObject>();
  set["combiner"] = "NONE";
  JsonObject setA = set["clauseA"].to<JsonObject>();
  JsonObject setSrc = setA["source"].to<JsonObject>();
  setSrc["cardId"] = 0;
  setSrc["field"] = "logicalState";
  setSrc["type"] = "BOOL";
  setA["operator"] = "EQ";
  setA["threshold"] = 1;
  JsonObject reset = cfg["reset"].to<JsonObject>();
  reset["combiner"] = "NONE";
  JsonObject resetA = reset["clauseA"].to<JsonObject>();
  JsonObject resetSrc = resetA["source"].to<JsonObject>();
  resetSrc["cardId"] = 0;
  resetSrc["field"] = "logicalState";
  resetSrc["type"] = "BOOL";
  resetA["operator"] = "EQ";
  resetA["threshold"] = 0;

  V3CardConfig out = {};
  String reason;
  bool ok = parseV3CardToTyped(card, sourceMap, kTotalCards, kDoStart, kAiStart,
                               kSioStart, kMathStart, kRtcStart, out, reason);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(0, out.cardId);
  TEST_ASSERT_EQUAL(static_cast<int>(V3CardFamily::DI),
                    static_cast<int>(out.family));
  TEST_ASSERT_EQUAL(Mode_DI_Rising, out.di.edgeMode);
}

void test_parse_rejects_card_type_slot_mismatch() {
  logicCardType sourceMap[kTotalCards] = {};
  seedSourceMap(sourceMap);

  JsonDocument doc;
  JsonObject card = doc.to<JsonObject>();
  card["cardId"] = 8;  // AI slot
  card["cardType"] = "DO";
  card["config"].to<JsonObject>();

  V3CardConfig out = {};
  String reason;
  bool ok = parseV3CardToTyped(card, sourceMap, kTotalCards, kDoStart, kAiStart,
                               kSioStart, kMathStart, kRtcStart, out, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_parse_rejects_invalid_mission_state_threshold() {
  logicCardType sourceMap[kTotalCards] = {};
  seedSourceMap(sourceMap);

  JsonDocument doc;
  JsonObject card = doc.to<JsonObject>();
  card["cardId"] = 4;
  card["cardType"] = "DO";
  JsonObject cfg = card["config"].to<JsonObject>();
  cfg["channel"] = 0;
  cfg["mode"] = "Normal";
  cfg["delayBeforeON"] = 1;
  cfg["onDuration"] = 1;
  cfg["repeatCount"] = 1;
  JsonObject set = cfg["set"].to<JsonObject>();
  set["combiner"] = "NONE";
  JsonObject setA = set["clauseA"].to<JsonObject>();
  JsonObject setSrc = setA["source"].to<JsonObject>();
  setSrc["cardId"] = 4;
  setSrc["field"] = "missionState";
  setSrc["type"] = "STATE";
  setA["operator"] = "EQ";
  setA["threshold"] = "PAUSED";
  JsonObject reset = cfg["reset"].to<JsonObject>();
  reset["combiner"] = "NONE";
  JsonObject resetA = reset["clauseA"].to<JsonObject>();
  JsonObject resetSrc = resetA["source"].to<JsonObject>();
  resetSrc["cardId"] = 4;
  resetSrc["field"] = "logicalState";
  resetSrc["type"] = "BOOL";
  resetA["operator"] = "EQ";
  resetA["threshold"] = 0;

  V3CardConfig out = {};
  String reason;
  bool ok = parseV3CardToTyped(card, sourceMap, kTotalCards, kDoStart, kAiStart,
                               kSioStart, kMathStart, kRtcStart, out, reason);
  TEST_ASSERT_FALSE(ok);
}

void test_parse_rtc_card_success() {
  logicCardType sourceMap[kTotalCards] = {};
  seedSourceMap(sourceMap);

  JsonDocument doc;
  JsonObject card = doc.to<JsonObject>();
  card["cardId"] = 16;
  card["cardType"] = "RTC";
  JsonObject cfg = card["config"].to<JsonObject>();
  JsonObject schedule = cfg["schedule"].to<JsonObject>();
  schedule["hour"] = 9;
  schedule["minute"] = 30;
  cfg["triggerDuration"] = 60000;

  V3CardConfig out = {};
  String reason;
  bool ok = parseV3CardToTyped(card, sourceMap, kTotalCards, kDoStart, kAiStart,
                               kSioStart, kMathStart, kRtcStart, out, reason);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(static_cast<int>(V3CardFamily::RTC),
                    static_cast<int>(out.family));
  TEST_ASSERT_EQUAL_UINT8(9, out.rtc.hour);
  TEST_ASSERT_EQUAL_UINT8(30, out.rtc.minute);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_parse_di_card_success);
  RUN_TEST(test_parse_rejects_card_type_slot_mismatch);
  RUN_TEST(test_parse_rejects_invalid_mission_state_threshold);
  RUN_TEST(test_parse_rtc_card_success);
  return UNITY_END();
}

