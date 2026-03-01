#include <unity.h>

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
  V3CardConfig out[kTotalCards] = {};
  V3RtcScheduleChannel rtcOut[1] = {};
  String reason;
  const char* errorCode = "VALIDATION_FAILED";
  bool ok = normalizeV3ConfigRequestTyped(
      req.as<JsonObjectConst>(), layout, "2.0", "2.0.0", baseline, kTotalCards,
      out, kTotalCards, rtcOut, 0, reason, errorCode);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(static_cast<int>(V3CardFamily::DI),
                    static_cast<int>(out[0].family));
  TEST_ASSERT_EQUAL(static_cast<int>(V3CardFamily::DO),
                    static_cast<int>(out[1].family));
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

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_normalize_service_rejects_invalid_api_version);
  RUN_TEST(test_normalize_service_accepts_valid_di_do_payload);
  RUN_TEST(test_build_legacy_cards_from_typed_uses_baseline);
  return UNITY_END();
}
