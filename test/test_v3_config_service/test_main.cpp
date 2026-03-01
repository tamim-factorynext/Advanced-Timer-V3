#include <unity.h>

#include "../../src/kernel/enum_codec.cpp"
#include "../../src/kernel/v3_condition_rules.cpp"
#include "../../src/kernel/v3_card_bridge.cpp"
#include "../../src/kernel/v3_config_sanitize.cpp"
#include "../../src/kernel/v3_payload_rules.cpp"
#include "../../src/kernel/v3_typed_card_parser.cpp"
#include "../../src/kernel/v3_typed_config_rules.cpp"
#include "../../src/storage/v3_normalizer.cpp"
#include "../../src/storage/v3_config_service.cpp"

namespace {
constexpr uint8_t kTotalCards = 2;
constexpr uint8_t kDoStart = 1;
constexpr uint8_t kAiStart = 2;
constexpr uint8_t kSioStart = 2;
constexpr uint8_t kMathStart = 2;
constexpr uint8_t kRtcStart = 2;

void seedBaseline(LogicCard* cards) {
  cards[0] = {};
  cards[0].id = 0;
  cards[0].type = DigitalInput;
  cards[0].index = 0;
  cards[0].mode = Mode_DI_Rising;
  cards[0].setA_ID = 0;
  cards[0].setB_ID = 0;
  cards[0].resetA_ID = 0;
  cards[0].resetB_ID = 0;
  cards[0].setA_Operator = Op_AlwaysFalse;
  cards[0].setB_Operator = Op_AlwaysFalse;
  cards[0].resetA_Operator = Op_AlwaysFalse;
  cards[0].resetB_Operator = Op_AlwaysFalse;

  cards[1] = {};
  cards[1].id = 1;
  cards[1].type = DigitalOutput;
  cards[1].index = 0;
  cards[1].mode = Mode_DO_Normal;
  cards[1].setA_ID = 0;
  cards[1].setB_ID = 0;
  cards[1].resetA_ID = 0;
  cards[1].resetB_ID = 0;
  cards[1].setA_Operator = Op_AlwaysFalse;
  cards[1].setB_Operator = Op_AlwaysFalse;
  cards[1].resetA_Operator = Op_AlwaysFalse;
  cards[1].resetB_Operator = Op_AlwaysFalse;
}

void writeSimpleCondition(JsonObject block, uint8_t sourceId, uint8_t threshold) {
  block["combiner"] = "NONE";
  JsonObject clauseA = block["clauseA"].to<JsonObject>();
  JsonObject src = clauseA["source"].to<JsonObject>();
  src["cardId"] = sourceId;
  src["field"] = "logicalState";
  src["type"] = "BOOL";
  clauseA["operator"] = "EQ";
  clauseA["threshold"] = threshold;
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_normalize_service_rejects_invalid_api_version() {
  LogicCard baseline[kTotalCards] = {};
  seedBaseline(baseline);

  JsonDocument req;
  req["apiVersion"] = "9.9";
  req["schemaVersion"] = "2.0.0";
  JsonObject config = req["config"].to<JsonObject>();
  config["cards"].to<JsonArray>();

  V3CardLayout layout = {kTotalCards, kDoStart, kAiStart, kSioStart, kMathStart,
                         kRtcStart};
  V3CardConfig out[kTotalCards] = {};
  V3RtcScheduleChannel rtcOut[1] = {};
  String reason;
  const char* errorCode = "VALIDATION_FAILED";
  bool ok = normalizeV3ConfigRequestTyped(
      req.as<JsonObjectConst>(), layout, "2.0", "2.0.0", baseline, kTotalCards,
      out, kTotalCards, rtcOut, 0, reason, errorCode);
  TEST_ASSERT_FALSE(ok);
}

void test_normalize_service_accepts_valid_di_do_payload() {
  LogicCard baseline[kTotalCards] = {};
  seedBaseline(baseline);

  JsonDocument req;
  req["requestId"] = "r1";
  req["apiVersion"] = "2.0";
  req["schemaVersion"] = "2.0.0";
  JsonObject config = req["config"].to<JsonObject>();
  JsonArray cards = config["cards"].to<JsonArray>();

  JsonObject di = cards.add<JsonObject>();
  di["cardId"] = 0;
  di["cardType"] = "DI";
  di["enabled"] = true;
  di["faultPolicy"] = "WARN";
  JsonObject diCfg = di["config"].to<JsonObject>();
  diCfg["channel"] = 0;
  diCfg["invert"] = false;
  diCfg["debounceTime"] = 10;
  diCfg["edgeMode"] = "RISING";
  writeSimpleCondition(diCfg["set"].to<JsonObject>(), 0, 1);
  writeSimpleCondition(diCfg["reset"].to<JsonObject>(), 0, 0);

  JsonObject dout = cards.add<JsonObject>();
  dout["cardId"] = 1;
  dout["cardType"] = "DO";
  dout["enabled"] = true;
  dout["faultPolicy"] = "WARN";
  JsonObject doCfg = dout["config"].to<JsonObject>();
  doCfg["channel"] = 0;
  doCfg["mode"] = "Normal";
  doCfg["delayBeforeON"] = 50;
  doCfg["onDuration"] = 100;
  doCfg["repeatCount"] = 1;
  writeSimpleCondition(doCfg["set"].to<JsonObject>(), 0, 1);
  writeSimpleCondition(doCfg["reset"].to<JsonObject>(), 0, 0);

  V3CardLayout layout = {kTotalCards, kDoStart, kAiStart, kSioStart, kMathStart,
                         kRtcStart};
  V3ConfigContext context = {};
  String reason;
  const char* errorCode = "VALIDATION_FAILED";
  bool ok = normalizeV3ConfigRequestContext(
      req.as<JsonObjectConst>(), layout, "2.0", "2.0.0", baseline, kTotalCards,
      0, context, reason, errorCode);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(static_cast<int>(V3CardFamily::DI),
                    static_cast<int>(context.typedCards[0].family));
  TEST_ASSERT_EQUAL(static_cast<int>(V3CardFamily::DO),
                    static_cast<int>(context.typedCards[1].family));
  TEST_ASSERT_EQUAL_UINT32(kTotalCards, context.typedCount);
}

void test_build_legacy_cards_from_typed_uses_baseline() {
  LogicCard baseline[kTotalCards] = {};
  seedBaseline(baseline);
  baseline[0].hwPin = 13;
  baseline[1].hwPin = 26;

  V3CardConfig typed[kTotalCards] = {};
  typed[0].cardId = 0;
  typed[0].family = V3CardFamily::DI;
  typed[0].di.channel = 0;
  typed[0].di.invert = false;
  typed[0].di.debounceTimeMs = 20;
  typed[0].di.edgeMode = Mode_DI_Rising;
  typed[0].di.set.clauseAId = 0;
  typed[0].di.set.clauseAOperator = Op_AlwaysFalse;
  typed[0].di.set.clauseAThreshold = 0;
  typed[0].di.set.clauseBId = 0;
  typed[0].di.set.clauseBOperator = Op_AlwaysFalse;
  typed[0].di.set.clauseBThreshold = 0;
  typed[0].di.set.combiner = Combine_None;
  typed[0].di.reset = typed[0].di.set;

  typed[1].cardId = 1;
  typed[1].family = V3CardFamily::DO;
  typed[1].dout.channel = 0;
  typed[1].dout.mode = Mode_DO_Normal;
  typed[1].dout.delayBeforeOnMs = 100;
  typed[1].dout.onDurationMs = 200;
  typed[1].dout.repeatCount = 1;
  typed[1].dout.set = typed[0].di.set;
  typed[1].dout.reset = typed[0].di.set;

  LogicCard out[kTotalCards] = {};
  String reason;
  bool ok = buildLegacyCardsFromTypedWithBaseline(
      typed, kTotalCards, baseline, kTotalCards, out, reason);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(13, out[0].hwPin);
  TEST_ASSERT_EQUAL_UINT8(26, out[1].hwPin);
  TEST_ASSERT_EQUAL_UINT32(20, out[0].setting1);
  TEST_ASSERT_EQUAL_UINT32(100, out[1].setting1);
}

void test_apply_rtc_schedule_channels_from_config_copies_fields() {
  V3RtcScheduleChannel source[2] = {};
  source[0].enabled = true;
  source[0].year = 2026;
  source[0].month = 3;
  source[0].day = 1;
  source[0].weekday = 0;
  source[0].hour = 12;
  source[0].minute = 34;
  source[0].rtcCardId = 16;
  source[1].enabled = false;
  source[1].year = -1;
  source[1].month = -1;
  source[1].day = -1;
  source[1].weekday = -1;
  source[1].hour = 8;
  source[1].minute = 0;
  source[1].rtcCardId = 17;

  V3RtcScheduleChannel target[2] = {};
  applyRtcScheduleChannelsFromConfig(source, 2, target, 2);

  TEST_ASSERT_TRUE(target[0].enabled);
  TEST_ASSERT_EQUAL_INT16(2026, target[0].year);
  TEST_ASSERT_EQUAL_INT8(3, target[0].month);
  TEST_ASSERT_EQUAL_INT8(1, target[0].day);
  TEST_ASSERT_EQUAL_INT8(0, target[0].weekday);
  TEST_ASSERT_EQUAL_INT8(12, target[0].hour);
  TEST_ASSERT_EQUAL_INT8(34, target[0].minute);
  TEST_ASSERT_EQUAL_UINT8(16, target[0].rtcCardId);
  TEST_ASSERT_FALSE(target[1].enabled);
  TEST_ASSERT_EQUAL_UINT8(17, target[1].rtcCardId);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_normalize_service_rejects_invalid_api_version);
  RUN_TEST(test_normalize_service_accepts_valid_di_do_payload);
  RUN_TEST(test_build_legacy_cards_from_typed_uses_baseline);
  RUN_TEST(test_apply_rtc_schedule_channels_from_config_copies_fields);
  return UNITY_END();
}
