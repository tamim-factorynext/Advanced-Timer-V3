/**********************************************************************************************
 *
 * ADVANCED TIMER FIRMWARE NOTES
 *
 * Canonical behavioral contracts live in requirements-v3-contract.md and docs/*.
 * Legacy V2 contract archive: docs/legacy/v2-poc-contract.md.
 *
 * This file should keep only implementation-local comments.
 * Do not duplicate long-form architecture contracts here.
 * Developed by Yahya Tamim
 **********************************************************************************************/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <RTClib.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <cstring>

#include "control/command_dto.h"
#include "kernel/card_model.h"
#include "kernel/enum_codec.h"
#include "kernel/v3_card_bridge.h"
#include "kernel/v3_condition_rules.h"
#include "kernel/v3_card_types.h"
#include "portal/routes.h"
#include "runtime/shared_snapshot.h"
#include "runtime/snapshot_json.h"
#include "storage/config_lifecycle.h"

const uint8_t DI_Pins[] = {13, 12, 14, 27};  // Digital Input pins
const uint8_t DO_Pins[] = {26, 25, 33, 32};  // Digital Output pins
const uint8_t AI_Pins[] = {35, 34};          // Analog Input pins
const uint8_t SIO_Pins[] = {255, 255, 255, 255};
// SoftIO has no physical pins, so we can use 255 as a placeholder for "virtual"
const uint8_t NUM_RTC_SCHED_CHANNELS = 2;

// --- Card counts (can be changed later) ---
const uint8_t NUM_DI = sizeof(DI_Pins) / sizeof(DI_Pins[0]);
const uint8_t NUM_DO = sizeof(DO_Pins) / sizeof(DO_Pins[0]);
const uint8_t NUM_AI = sizeof(AI_Pins) / sizeof(AI_Pins[0]);
const uint8_t NUM_SIO = sizeof(SIO_Pins) / sizeof(SIO_Pins[0]);
const uint8_t NUM_MATH = 2;
const uint8_t NUM_RTC = NUM_RTC_SCHED_CHANNELS;

const uint8_t TOTAL_CARDS =
    NUM_DI + NUM_DO + NUM_AI + NUM_SIO + NUM_MATH + NUM_RTC;
struct SharedRuntimeSnapshot : SharedRuntimeSnapshotT<TOTAL_CARDS> {};

const uint8_t DI_START = 0;
const uint8_t DO_START = DI_START + NUM_DI;
const uint8_t AI_START = DO_START + NUM_DO;
const uint8_t SIO_START = AI_START + NUM_AI;
const uint8_t MATH_START = SIO_START + NUM_SIO;
const uint8_t RTC_START = MATH_START + NUM_MATH;
const char* kConfigPath = "/config.json";
const char* kStagedConfigPath = "/config_staged.json";
const char* kLkgConfigPath = "/config_lkg.json";
const char* kSlot1ConfigPath = "/config_slot1.json";
const char* kSlot2ConfigPath = "/config_slot2.json";
const char* kSlot3ConfigPath = "/config_slot3.json";
const char* kFactoryConfigPath = "/config_factory.json";
const char* kPortalSettingsPath = "/portal_settings.json";
const uint32_t kDefaultScanIntervalMs = 500;
const uint32_t kMinScanIntervalMs = 10;
const uint32_t kMaxScanIntervalMs = 1000;
const char* kMasterSsid = "advancedtimer";
const char* kMasterPassword = "12345678";
const char* kDefaultUserSsid = "FactoryNext";
const char* kDefaultUserPassword = "FactoryNext20$22#";
const uint32_t MASTER_WIFI_TIMEOUT_MS = 2000;
const uint32_t USER_WIFI_TIMEOUT_MS = 180000;
const char* kApiVersion = "2.0";
const char* kSchemaVersion = "2.0.0";

#ifndef LOGIC_ENGINE_DEBUG
#define LOGIC_ENGINE_DEBUG 0
#endif

#if LOGIC_ENGINE_DEBUG
#define LOGIC_DEBUG_PRINTLN(x) Serial.println(x)
#else
#define LOGIC_DEBUG_PRINTLN(x) \
  do {                         \
  } while (0)
#endif

LogicCard logicCards[TOTAL_CARDS] = {};
bool gPrevSetCondition[TOTAL_CARDS] = {};
bool gPrevDISample[TOTAL_CARDS] = {};
bool gPrevDIPrimed[TOTAL_CARDS] = {};
bool gCardSetResult[TOTAL_CARDS] = {};
bool gCardResetResult[TOTAL_CARDS] = {};
bool gCardResetOverride[TOTAL_CARDS] = {};
uint32_t gCardEvalCounter[TOTAL_CARDS] = {};

QueueHandle_t gKernelCommandQueue = nullptr;
TaskHandle_t gCore0TaskHandle = nullptr;
TaskHandle_t gCore1TaskHandle = nullptr;
portMUX_TYPE gSnapshotMux = portMUX_INITIALIZER_UNLOCKED;
SharedRuntimeSnapshot gSharedSnapshot = {};
WebServer gPortalServer(80);
WebSocketsServer gWsServer(81);
char gUserSsid[33] = {};
char gUserPassword[65] = {};
bool gPortalReconnectRequested = false;
bool gPortalServerInitialized = false;
bool gWsServerInitialized = false;
volatile bool gKernelPauseRequested = false;
volatile bool gKernelPaused = false;
uint32_t gConfigVersionCounter = 1;
char gActiveVersion[16] = "v1";
char gLkgVersion[16] = "";
char gSlot1Version[16] = "";
char gSlot2Version[16] = "";
char gSlot3Version[16] = "";

runMode gRunMode = RUN_NORMAL;
uint16_t gScanCursor = 0;
bool gStepRequested = false;
bool gBreakpointPaused = false;
bool gTestModeActive = false;
bool gGlobalOutputMask = false;

bool gCardBreakpoint[TOTAL_CARDS] = {};
bool gCardOutputMask[TOTAL_CARDS] = {};
inputSourceMode gCardInputSource[TOTAL_CARDS] = {};
uint32_t gCardForcedAIValue[TOTAL_CARDS] = {};
uint32_t gScanIntervalMs = kDefaultScanIntervalMs;
uint32_t gLastCompleteScanUs = 0;
uint32_t gMaxCompleteScanUs = 0;
uint32_t gScanBudgetUs = kDefaultScanIntervalMs * 1000;
uint32_t gScanOverrunCount = 0;
bool gScanOverrunLast = false;
uint16_t gKernelQueueDepth = 0;
uint16_t gKernelQueueHighWaterMark = 0;
uint16_t gKernelQueueCapacity = 0;
uint32_t gCommandLatencyLastUs = 0;
uint32_t gCommandLatencyMaxUs = 0;
RTC_Millis gRtcClock;
bool gRtcClockInitialized = false;
int32_t gRtcLastMinuteKey = -1;
uint32_t gRtcMinuteTickCount = 0;
uint32_t gRtcIntentEnqueueCount = 0;
uint32_t gRtcIntentEnqueueFailCount = 0;
uint32_t gRtcLastEvalMs = 0;

struct RtcScheduleChannel {
  bool enabled;
  int16_t year;  // -1 means wildcard
  int8_t month;  // -1 means wildcard
  int8_t day;    // -1 means wildcard
  int8_t weekday;  // -1 means wildcard
  int8_t hour;     // -1 means wildcard
  int8_t minute;   // -1 means wildcard
  uint8_t rtcCardId;
};

RtcScheduleChannel gRtcScheduleChannels[NUM_RTC_SCHED_CHANNELS] = {
    {false, -1, -1, -1, -1, -1, -1, RTC_START},
    {false, -1, -1, -1, -1, -1, -1, static_cast<uint8_t>(RTC_START + 1)},
};

bool isOutputMasked(uint8_t cardId);
uint8_t scanOrderCardIdFromCursor(uint16_t cursor);
bool connectWiFiWithPolicy();
bool applyCommand(JsonObjectConst command);
bool setRtcCardStateCommand(uint8_t cardId, bool state);
void updateSharedRuntimeSnapshot(uint32_t nowMs, bool incrementSeq);
void serializeCardsToArray(const LogicCard* sourceCards, JsonArray& array);
void initializeCardArraySafeDefaults(LogicCard* cards);
bool deserializeCardsFromArray(JsonArrayConst array, LogicCard* outCards);
bool validateConfigCardsArray(JsonArrayConst array, String& reason);
void serviceRtcMinuteScheduler(uint32_t nowMs);
void writeConfigResultResponse(int statusCode, bool ok, const char* requestId,
                               const char* errorCode, const String& message,
                               JsonObject* extra = nullptr);
bool normalizeConfigRequest(JsonObjectConst root, JsonDocument& normalizedDoc,
                            JsonArrayConst& outCards, String& reason,
                            const char*& outErrorCode,
                            bool& usedLegacyBridge);
void serializeCardsToV3Array(const LogicCard* sourceCards, JsonArray& cards);
void buildV3ConfigEnvelope(const LogicCard* sourceCards, JsonDocument& doc,
                           const char* configId, const char* requestId);

void serializeCardToJson(const LogicCard& card, JsonObject& json) {
  json["id"] = card.id;
  json["type"] = toString(card.type);
  json["index"] = card.index;
  json["hwPin"] = card.hwPin;
  json["invert"] = card.invert;

  json["setting1"] = card.setting1;
  json["setting2"] = card.setting2;
  if (card.type == AnalogInput) {
    json["setting3"] = static_cast<double>(card.setting3) / 1000.0;
  } else {
    json["setting3"] = card.setting3;
  }

  json["logicalState"] = card.logicalState;
  json["physicalState"] = card.physicalState;
  json["triggerFlag"] = card.triggerFlag;
  json["currentValue"] = card.currentValue;
  json["startOnMs"] = card.startOnMs;
  json["startOffMs"] = card.startOffMs;
  json["repeatCounter"] = card.repeatCounter;

  json["mode"] = toString(card.mode);
  json["state"] = toString(card.state);

  json["setA_ID"] = card.setA_ID;
  json["setA_Operator"] = toString(card.setA_Operator);
  json["setA_Threshold"] = card.setA_Threshold;
  json["setB_ID"] = card.setB_ID;
  json["setB_Operator"] = toString(card.setB_Operator);
  json["setB_Threshold"] = card.setB_Threshold;
  json["setCombine"] = toString(card.setCombine);

  json["resetA_ID"] = card.resetA_ID;
  json["resetA_Operator"] = toString(card.resetA_Operator);
  json["resetA_Threshold"] = card.resetA_Threshold;
  json["resetB_ID"] = card.resetB_ID;
  json["resetB_Operator"] = toString(card.resetB_Operator);
  json["resetB_Threshold"] = card.resetB_Threshold;
  json["resetCombine"] = toString(card.resetCombine);
}

void deserializeCardFromJson(JsonVariantConst jsonVariant, LogicCard& card) {
  if (!jsonVariant.is<JsonObjectConst>()) return;
  JsonObjectConst json = jsonVariant.as<JsonObjectConst>();
  LogicCard before = card;

  card.id = json["id"] | card.id;
  const char* rawType = json["type"].as<const char*>();
  logicCardType parsedType = before.type;
  bool typeOk = tryParseLogicCardType(rawType, parsedType);
  card.type = typeOk ? parsedType : before.type;
  card.index = json["index"] | card.index;
  card.hwPin = json["hwPin"] | card.hwPin;
  card.invert = json["invert"] | card.invert;

  card.setting1 = json["setting1"] | card.setting1;
  card.setting2 = json["setting2"] | card.setting2;
  if (card.type == AnalogInput) {
    const double currentAlpha = static_cast<double>(card.setting3) / 1000.0;
    const double parsed = json["setting3"] | currentAlpha;
    if (parsed >= 0.0 && parsed <= 1.0) {
      const double scaled = parsed * 1000.0;
      card.setting3 = static_cast<uint32_t>(scaled + 0.5);
    } else {
      // Backward compatibility: accept legacy milliunit payloads.
      int32_t legacy = static_cast<int32_t>(parsed);
      if (legacy < 0) legacy = 0;
      if (legacy > 1000) legacy = 1000;
      card.setting3 = static_cast<uint32_t>(legacy);
    }
  } else {
    card.setting3 = json["setting3"] | card.setting3;
  }

  card.logicalState = json["logicalState"] | card.logicalState;
  card.physicalState = json["physicalState"] | card.physicalState;
  card.triggerFlag = json["triggerFlag"] | card.triggerFlag;
  card.currentValue = json["currentValue"] | card.currentValue;
  card.startOnMs = json["startOnMs"] | card.startOnMs;
  card.startOffMs = json["startOffMs"] | card.startOffMs;
  card.repeatCounter = json["repeatCounter"] | card.repeatCounter;

  const char* rawMode = json["mode"].as<const char*>();
  cardMode parsedMode = before.mode;
  bool modeOk = tryParseCardMode(rawMode, parsedMode);
  card.mode = modeOk ? parsedMode : before.mode;

  const char* rawState = json["state"].as<const char*>();
  cardState parsedState = before.state;
  bool stateOk = tryParseCardState(rawState, parsedState);
  card.state = stateOk ? parsedState : before.state;

  card.setA_ID = json["setA_ID"] | card.setA_ID;
  const char* rawSetA = json["setA_Operator"].as<const char*>();
  logicOperator parsedSetA = before.setA_Operator;
  bool setAOk = tryParseLogicOperator(rawSetA, parsedSetA);
  card.setA_Operator = setAOk ? parsedSetA : before.setA_Operator;
  card.setA_Threshold = json["setA_Threshold"] | card.setA_Threshold;
  card.setB_ID = json["setB_ID"] | card.setB_ID;
  const char* rawSetB = json["setB_Operator"].as<const char*>();
  logicOperator parsedSetB = before.setB_Operator;
  bool setBOk = tryParseLogicOperator(rawSetB, parsedSetB);
  card.setB_Operator = setBOk ? parsedSetB : before.setB_Operator;
  card.setB_Threshold = json["setB_Threshold"] | card.setB_Threshold;
  const char* rawSetCombine = json["setCombine"].as<const char*>();
  combineMode parsedSetCombine = before.setCombine;
  bool setCombineOk = tryParseCombineMode(rawSetCombine, parsedSetCombine);
  card.setCombine = setCombineOk ? parsedSetCombine : before.setCombine;

  card.resetA_ID = json["resetA_ID"] | card.resetA_ID;
  const char* rawResetA = json["resetA_Operator"].as<const char*>();
  logicOperator parsedResetA = before.resetA_Operator;
  bool resetAOk = tryParseLogicOperator(rawResetA, parsedResetA);
  card.resetA_Operator = resetAOk ? parsedResetA : before.resetA_Operator;
  card.resetA_Threshold = json["resetA_Threshold"] | card.resetA_Threshold;
  card.resetB_ID = json["resetB_ID"] | card.resetB_ID;
  const char* rawResetB = json["resetB_Operator"].as<const char*>();
  logicOperator parsedResetB = before.resetB_Operator;
  bool resetBOk = tryParseLogicOperator(rawResetB, parsedResetB);
  card.resetB_Operator = resetBOk ? parsedResetB : before.resetB_Operator;
  card.resetB_Threshold = json["resetB_Threshold"] | card.resetB_Threshold;
  const char* rawResetCombine = json["resetCombine"].as<const char*>();
  combineMode parsedResetCombine = before.resetCombine;
  bool resetCombineOk =
      tryParseCombineMode(rawResetCombine, parsedResetCombine);
  card.resetCombine = resetCombineOk ? parsedResetCombine : before.resetCombine;
}

void initializeCardSafeDefaults(LogicCard& card, uint8_t globalId) {
  card.id = globalId;
  card.invert = false;
  card.setting1 = 0;
  card.setting2 = 0;
  card.setting3 = 0;
  card.logicalState = false;
  card.physicalState = false;
  card.triggerFlag = false;
  card.currentValue = 0;
  card.startOnMs = 0;
  card.startOffMs = 0;
  card.repeatCounter = 0;

  card.setA_ID = globalId;
  card.setB_ID = globalId;
  card.resetA_ID = globalId;
  card.resetB_ID = globalId;
  card.setA_Operator = Op_AlwaysFalse;
  card.setB_Operator = Op_AlwaysFalse;
  card.resetA_Operator = Op_AlwaysFalse;
  card.resetB_Operator = Op_AlwaysFalse;
  card.setA_Threshold = 0;
  card.setB_Threshold = 0;
  card.resetA_Threshold = 0;
  card.resetB_Threshold = 0;
  card.setCombine = Combine_None;
  card.resetCombine = Combine_None;

  if (globalId < DO_START) {
    card.type = DigitalInput;
    card.index = globalId - DI_START;
    card.hwPin = DI_Pins[card.index];
    // DI defaults: debounced edge input behavior.
    card.setting1 = 50;  // debounce window
    card.setting2 = 0;   // reserved
    card.setting3 = 0;   // reserved
    card.mode = Mode_DI_Rising;
    card.state = State_DI_Idle;
    return;
  }

  if (globalId < AI_START) {
    card.type = DigitalOutput;
    card.index = globalId - DO_START;
    card.hwPin = DO_Pins[card.index];
    // DO defaults: safe one-shot profile, but disabled by condition defaults.
    card.setting1 = 1000;  // delay before ON
    card.setting2 = 1000;  // ON duration
    card.setting3 = 1;     // one cycle
    card.mode = Mode_DO_Normal;
    card.state = State_DO_Idle;
    return;
  }

  if (globalId < SIO_START) {
    card.type = AnalogInput;
    card.index = globalId - AI_START;
    card.hwPin = AI_Pins[card.index];
    // AI defaults: raw ADC range with moderate smoothing and 0..100.00 output.
    card.setting1 = 0;        // input minimum
    card.setting2 = 4095;     // input maximum
    card.setting3 = 250;      // EMA alpha = 0.25 (stored as 250/1000)
    card.startOnMs = 0;       // output minimum (centiunits)
    card.startOffMs = 10000;  // output maximum (centiunits)
    card.mode = Mode_AI_Continuous;
    card.state = State_AI_Streaming;
    return;
  }

  if (globalId < MATH_START) {
    card.type = SoftIO;
    card.index = globalId - SIO_START;
    card.hwPin = SIO_Pins[card.index];
    // SoftIO defaults follow DO defaults (virtual output).
    card.setting1 = 1000;
    card.setting2 = 1000;
    card.setting3 = 1;
    card.mode = Mode_DO_Normal;
    card.state = State_DO_Idle;
    return;
  }

  if (globalId < RTC_START) {
    card.type = MathCard;
    card.index = globalId - MATH_START;
    card.hwPin = 255;
    // MATH defaults: enabled by set/reset gating, simple arithmetic seed.
    card.setting1 = 0;      // inputA/future source config
    card.setting2 = 0;      // inputB/future source config
    card.setting3 = 0;      // fallback value
    card.startOnMs = 0;     // clamp min
    card.startOffMs = 0;    // clamp max (disabled when max < min)
    card.mode = Mode_None;  // reserved for V3 typed math modes
    card.state = State_None;
    return;
  }

  card.type = RtcCard;
  card.index = globalId - RTC_START;
  card.hwPin = 255;
  // RTC defaults: minute scheduler sources with inactive output.
  card.setting1 = 60000;  // trigger window (ms) bridge until typed schema lands
  card.setting2 = 0;
  card.setting3 = 0;
  card.mode = Mode_None;
  card.state = State_None;
}

void initializeAllCardsSafeDefaults() {
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    initializeCardSafeDefaults(logicCards[i], i);
  }
}

bool saveLogicCardsToLittleFS() {
  return saveCardsToPath(kConfigPath, logicCards);
}

bool loadLogicCardsFromLittleFS() {
  LogicCard loaded[TOTAL_CARDS];
  if (!loadCardsFromPath(kConfigPath, loaded)) return false;
  memcpy(logicCards, loaded, sizeof(logicCards));
  return true;
}

void printLogicCardsJsonToSerial(const char* label) {
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    JsonObject obj = array.add<JsonObject>();
    serializeCardToJson(logicCards[i], obj);
  }

  Serial.println(label);
  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void copySharedRuntimeSnapshot(SharedRuntimeSnapshot& outSnapshot) {
  portENTER_CRITICAL(&gSnapshotMux);
  outSnapshot = gSharedSnapshot;
  portEXIT_CRITICAL(&gSnapshotMux);
}

void appendRuntimeSnapshotCard(JsonArray& cards,
                               const SharedRuntimeSnapshot& snapshot,
                               uint8_t cardId) {
  const LogicCard& card = snapshot.cards[cardId];
  JsonObject node = cards.add<JsonObject>();
  node["id"] = card.id;
  node["type"] = toString(card.type);
  node["index"] = card.index;
  node["familyOrder"] = cardId;
  node["physicalState"] = card.physicalState;
  node["logicalState"] = card.logicalState;
  node["triggerFlag"] = card.triggerFlag;
  node["state"] = toString(card.state);
  node["mode"] = toString(card.mode);
  node["currentValue"] = card.currentValue;
  node["startOnMs"] = card.startOnMs;
  node["startOffMs"] = card.startOffMs;
  node["repeatCounter"] = card.repeatCounter;

  JsonObject forced = node["maskForced"].to<JsonObject>();
  forced["inputSource"] = toString(snapshot.inputSource[cardId]);
  forced["forcedAIValue"] = snapshot.forcedAIValue[cardId];
  forced["outputMaskLocal"] = snapshot.outputMaskLocal[cardId];
  forced["outputMasked"] =
      (snapshot.globalOutputMask || snapshot.outputMaskLocal[cardId]);
  node["breakpointEnabled"] = snapshot.breakpointEnabled[cardId];
  node["setResult"] = snapshot.setResult[cardId];
  node["resetResult"] = snapshot.resetResult[cardId];
  node["resetOverride"] = snapshot.resetOverride[cardId];
  node["evalCounter"] = snapshot.evalCounter[cardId];
  JsonObject debug = node["debug"].to<JsonObject>();
  debug["evalCounter"] = snapshot.evalCounter[cardId];
  debug["breakpointEnabled"] = snapshot.breakpointEnabled[cardId];
}

void serializeRuntimeSnapshot(JsonDocument& doc, uint32_t nowMs) {
  SharedRuntimeSnapshot snapshot = {};
  copySharedRuntimeSnapshot(snapshot);

  doc["type"] = "runtime_snapshot";
  doc["schemaVersion"] = 1;
  doc["tsMs"] = (snapshot.tsMs == 0) ? nowMs : snapshot.tsMs;
  doc["scanIntervalMs"] = gScanIntervalMs;
  doc["lastCompleteScanMs"] =
      static_cast<double>(snapshot.lastCompleteScanUs) / 1000.0;
  JsonObject metrics = doc["metrics"].to<JsonObject>();
  metrics["scanLastUs"] = snapshot.lastCompleteScanUs;
  metrics["scanMaxUs"] = snapshot.maxCompleteScanUs;
  metrics["scanBudgetUs"] = snapshot.scanBudgetUs;
  metrics["scanOverrunLast"] = snapshot.scanOverrunLast;
  metrics["scanOverrunCount"] = snapshot.scanOverrunCount;
  metrics["queueDepth"] = snapshot.kernelQueueDepth;
  metrics["queueHighWaterMark"] = snapshot.kernelQueueHighWaterMark;
  metrics["queueCapacity"] = snapshot.kernelQueueCapacity;
  metrics["commandLatencyLastUs"] = snapshot.commandLatencyLastUs;
  metrics["commandLatencyMaxUs"] = snapshot.commandLatencyMaxUs;
  metrics["rtcMinuteTickCount"] = snapshot.rtcMinuteTickCount;
  metrics["rtcIntentEnqueueCount"] = snapshot.rtcIntentEnqueueCount;
  metrics["rtcIntentEnqueueFailCount"] = snapshot.rtcIntentEnqueueFailCount;
  metrics["rtcLastEvalMs"] = snapshot.rtcLastEvalMs;
  doc["runMode"] = toString(snapshot.mode);
  doc["snapshotSeq"] = snapshot.seq;

  JsonObject testMode = doc["testMode"].to<JsonObject>();
  testMode["active"] = snapshot.testModeActive;
  testMode["outputMaskGlobal"] = snapshot.globalOutputMask;
  testMode["breakpointPaused"] = snapshot.breakpointPaused;
  testMode["scanCursor"] = snapshot.scanCursor;

  JsonArray cards = doc["cards"].to<JsonArray>();
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    appendRuntimeSnapshotCard(cards, snapshot, scanOrderCardIdFromCursor(i));
  }
}

bool waitForWiFiConnected(uint32_t timeoutMs) {
  uint32_t startMs = millis();
  while ((millis() - startMs) < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) return true;
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  return false;
}

bool connectWiFiWithPolicy() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  vTaskDelay(pdMS_TO_TICKS(50));

  WiFi.begin(kMasterSsid, kMasterPassword);
  if (waitForWiFiConnected(MASTER_WIFI_TIMEOUT_MS)) {
    Serial.print("WiFi connected via MASTER SSID. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  WiFi.disconnect(true, true);
  vTaskDelay(pdMS_TO_TICKS(50));

  WiFi.begin(gUserSsid, gUserPassword);
  if (waitForWiFiConnected(USER_WIFI_TIMEOUT_MS)) {
    Serial.print("WiFi connected via USER SSID. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi offline mode (master/user connect attempts failed)");
  return false;
}

void handleHttpRoot() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    gPortalServer.send(
        404, "text/plain",
        "index.html not found in LittleFS (/data upload needed)");
    return;
  }
  gPortalServer.streamFile(file, "text/html");
  file.close();
}

void handleHttpSettingsPage() {
  File file = LittleFS.open("/settings.html", "r");
  if (!file) {
    gPortalServer.send(
        404, "text/plain",
        "settings.html not found in LittleFS (/data upload needed)");
    return;
  }
  gPortalServer.streamFile(file, "text/html");
  file.close();
}

void handleHttpConfigPage() {
  File file = LittleFS.open("/config.html", "r");
  if (!file) {
    gPortalServer.send(
        404, "text/plain",
        "config.html not found in LittleFS (/data upload needed)");
    return;
  }
  gPortalServer.streamFile(file, "text/html");
  file.close();
}

void handleHttpGetSettings() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["masterSsid"] = kMasterSsid;
  doc["masterEditable"] = false;
  doc["userSsid"] = gUserSsid;
  doc["userPassword"] = gUserPassword;
  doc["scanIntervalMs"] = gScanIntervalMs;
  doc["scanIntervalMinMs"] = kMinScanIntervalMs;
  doc["scanIntervalMaxMs"] = kMaxScanIntervalMs;
  doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
  doc["wifiIp"] = WiFi.localIP().toString();
  doc["firmwareVersion"] = String(__DATE__) + " " + String(__TIME__);

  String body;
  serializeJson(doc, body);
  gPortalServer.send(200, "application/json", body);
}

void handleHttpSaveSettingsWiFi() {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, gPortalServer.arg("plain"));
  if (error || !doc.is<JsonObject>()) {
    gPortalServer.send(400, "application/json",
                       "{\"ok\":false,\"error\":\"INVALID_REQUEST\"}");
    return;
  }
  JsonObjectConst root = doc.as<JsonObjectConst>();
  const char* ssid = root["userSsid"] | "";
  const char* password = root["userPassword"] | "";

  if (strlen(ssid) == 0 || strlen(ssid) > 32 || strlen(password) > 64) {
    gPortalServer.send(400, "application/json",
                       "{\"ok\":false,\"error\":\"VALIDATION_FAILED\"}");
    return;
  }

  strncpy(gUserSsid, ssid, sizeof(gUserSsid) - 1);
  gUserSsid[sizeof(gUserSsid) - 1] = '\0';
  strncpy(gUserPassword, password, sizeof(gUserPassword) - 1);
  gUserPassword[sizeof(gUserPassword) - 1] = '\0';

  savePortalSettingsToLittleFS();
  gPortalServer.send(200, "application/json", "{\"ok\":true}");
}

void handleHttpSaveSettingsRuntime() {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, gPortalServer.arg("plain"));
  if (error || !doc.is<JsonObject>()) {
    gPortalServer.send(400, "application/json",
                       "{\"ok\":false,\"error\":\"INVALID_REQUEST\"}");
    return;
  }
  JsonObjectConst root = doc.as<JsonObjectConst>();
  const uint32_t requested = root["scanIntervalMs"] | 0;
  if (requested < kMinScanIntervalMs || requested > kMaxScanIntervalMs) {
    gPortalServer.send(400, "application/json",
                       "{\"ok\":false,\"error\":\"VALIDATION_FAILED\"}");
    return;
  }

  gScanIntervalMs = requested;
  savePortalSettingsToLittleFS();
  gPortalServer.send(200, "application/json", "{\"ok\":true}");
}

void handleHttpReconnectWiFi() {
  gPortalReconnectRequested = true;
  gPortalServer.send(200, "application/json", "{\"ok\":true}");
}

void handleHttpReboot() {
  gPortalServer.send(200, "application/json", "{\"ok\":true}");
  gPortalServer.client().stop();
  delay(200);
  ESP.restart();
}

void handleHttpSnapshot() {
  JsonDocument doc;
  serializeRuntimeSnapshot(doc, millis());
  String body;
  serializeJson(doc, body);
  gPortalServer.send(200, "application/json", body);
}

void handleHttpCommand() {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, gPortalServer.arg("plain"));
  if (error || !doc.is<JsonObject>()) {
    gPortalServer.send(400, "application/json",
                       "{\"ok\":false,\"error\":\"INVALID_REQUEST\"}");
    return;
  }

  bool ok = applyCommand(doc.as<JsonObjectConst>());
  if (!ok) {
    gPortalServer.send(400, "application/json",
                       "{\"ok\":false,\"error\":\"COMMAND_REJECTED\"}");
    return;
  }

  gPortalServer.send(200, "application/json", "{\"ok\":true}");
}

void handleHttpGetActiveConfig() {
  JsonDocument doc;
  buildV3ConfigEnvelope(logicCards, doc, gActiveVersion, "");
  doc["status"] = "SUCCESS";
  doc["activeVersion"] = gActiveVersion;
  doc["errorCode"] = nullptr;
  String body;
  serializeJson(doc, body);
  gPortalServer.send(200, "application/json", body);
}

void handleHttpStagedSaveConfig() {
  JsonDocument request;
  DeserializationError parseError =
      deserializeJson(request, gPortalServer.arg("plain"));
  if (parseError || !request.is<JsonObjectConst>()) {
    writeConfigResultResponse(400, false, "", "INVALID_REQUEST", "invalid json");
    return;
  }

  JsonObjectConst root = request.as<JsonObjectConst>();
  JsonArrayConst cards;
  JsonDocument normalized;
  String reason;
  const char* errorCode = "VALIDATION_FAILED";
  bool usedLegacyBridge = false;
  const char* requestId = root["requestId"] | "";
  if (!normalizeConfigRequest(root, normalized, cards, reason, errorCode,
                              usedLegacyBridge)) {
    writeConfigResultResponse(400, false, requestId, errorCode, reason);
    return;
  }

  LogicCard stagedCards[TOTAL_CARDS];
  if (!deserializeCardsFromArray(cards, stagedCards)) {
    writeConfigResultResponse(500, false, requestId, "INTERNAL_ERROR",
                              "failed to normalize staged cards");
    return;
  }

  JsonDocument stagedDoc;
  buildV3ConfigEnvelope(stagedCards, stagedDoc, "staged", requestId);
  JsonObject bridge = stagedDoc["bridge"].to<JsonObject>();
  bridge["usedLegacyCardsBridge"] = usedLegacyBridge;
  bridge["bridgeVersion"] = 1;

  if (!writeJsonToPath(kStagedConfigPath, stagedDoc)) {
    writeConfigResultResponse(500, false, requestId, "COMMIT_FAILED",
                              "failed to save staged file");
    return;
  }

  JsonDocument extras;
  extras["stagedVersion"] = "staged";
  extras["usedLegacyCardsBridge"] = usedLegacyBridge;
  JsonObject extrasObj = extras.as<JsonObject>();
  writeConfigResultResponse(200, true, requestId, nullptr, "", &extrasObj);
}

void handleHttpStagedValidateConfig() {
  JsonDocument source;
  JsonDocument normalized;
  JsonArrayConst cards;
  String reason;
  const char* requestId = "";
  bool usedLegacyBridge = false;

  if (gPortalServer.hasArg("plain") &&
      gPortalServer.arg("plain").length() > 0) {
    DeserializationError parseError =
        deserializeJson(source, gPortalServer.arg("plain"));
    if (parseError || !source.is<JsonObjectConst>()) {
      writeConfigResultResponse(400, false, "", "INVALID_REQUEST", "invalid json");
      return;
    }
    JsonObjectConst root = source.as<JsonObjectConst>();
    requestId = root["requestId"] | "";
    const char* errorCode = "VALIDATION_FAILED";
    if (!normalizeConfigRequest(root, normalized, cards, reason, errorCode,
                                usedLegacyBridge)) {
      writeConfigResultResponse(400, false, requestId, errorCode, reason);
      return;
    }
  } else {
    if (!readJsonFromPath(kStagedConfigPath, source) ||
        !source.is<JsonObjectConst>()) {
      writeConfigResultResponse(404, false, "", "NOT_FOUND",
                                "no staged config available");
      return;
    }
    JsonObjectConst root = source.as<JsonObjectConst>();
    requestId = root["requestId"] | "";
    const char* errorCode = "VALIDATION_FAILED";
    if (!normalizeConfigRequest(root, normalized, cards, reason, errorCode,
                                usedLegacyBridge)) {
      writeConfigResultResponse(400, false, requestId, errorCode, reason);
      return;
    }
  }

  JsonDocument extras;
  JsonObject validation = extras["validation"].to<JsonObject>();
  validation["errors"].to<JsonArray>();
  validation["warnings"].to<JsonArray>();
  extras["usedLegacyCardsBridge"] = usedLegacyBridge;
  JsonObject extrasObj = extras.as<JsonObject>();
  writeConfigResultResponse(200, true, requestId, nullptr, "", &extrasObj);
}

bool rotateHistoryFiles() {
  if (!copyFileIfExists(kSlot2ConfigPath, kSlot3ConfigPath)) return false;
  if (!copyFileIfExists(kSlot1ConfigPath, kSlot2ConfigPath)) return false;
  if (!copyFileIfExists(kLkgConfigPath, kSlot1ConfigPath)) return false;
  if (!copyFileIfExists(kConfigPath, kLkgConfigPath)) return false;
  rotateHistoryVersions();
  return true;
}

void writeHistoryHead(JsonObject& head) {
  head["lkgVersion"] = gLkgVersion;
  head["slot1Version"] = gSlot1Version;
  head["slot2Version"] = gSlot2Version;
  head["slot3Version"] = gSlot3Version;
}

bool commitCards(JsonArrayConst cards, String& reason) {
  LogicCard nextCards[TOTAL_CARDS];
  if (!deserializeCardsFromArray(cards, nextCards)) {
    reason = "failed to parse cards";
    return false;
  }

  if (!rotateHistoryFiles()) {
    reason = "failed to rotate history slots";
    return false;
  }

  if (!saveCardsToPath(kConfigPath, nextCards)) {
    reason = "failed to persist active config";
    return false;
  }

  if (!applyCardsAsActiveConfig(nextCards)) {
    reason = "failed to apply active config to runtime";
    return false;
  }

  gConfigVersionCounter += 1;
  formatVersion(gActiveVersion, sizeof(gActiveVersion), gConfigVersionCounter);
  return true;
}

void handleHttpCommitConfig() {
  JsonDocument sourceDoc;
  JsonDocument normalized;
  JsonArrayConst cards;
  String reason;
  const char* requestId = "";
  bool usedLegacyBridge = false;
  const bool hasInlinePayload =
      gPortalServer.hasArg("plain") && gPortalServer.arg("plain").length() > 0;

  if (hasInlinePayload) {
    DeserializationError parseError =
        deserializeJson(sourceDoc, gPortalServer.arg("plain"));
    if (parseError || !sourceDoc.is<JsonObjectConst>()) {
      writeConfigResultResponse(400, false, "", "INVALID_REQUEST", "invalid json");
      return;
    }
    JsonObjectConst root = sourceDoc.as<JsonObjectConst>();
    requestId = root["requestId"] | "";
    const char* errorCode = "VALIDATION_FAILED";
    if (!normalizeConfigRequest(root, normalized, cards, reason, errorCode,
                                usedLegacyBridge)) {
      writeConfigResultResponse(400, false, requestId, errorCode, reason);
      return;
    }
  } else {
    if (!readJsonFromPath(kStagedConfigPath, sourceDoc) ||
        !sourceDoc.is<JsonObjectConst>()) {
      writeConfigResultResponse(404, false, "", "NOT_FOUND",
                                "no staged config available");
      return;
    }
    JsonObjectConst root = sourceDoc.as<JsonObjectConst>();
    requestId = root["requestId"] | "";
    const char* errorCode = "VALIDATION_FAILED";
    if (!normalizeConfigRequest(root, normalized, cards, reason, errorCode,
                                usedLegacyBridge)) {
      writeConfigResultResponse(400, false, requestId, errorCode, reason);
      return;
    }
  }

  if (!commitCards(cards, reason)) {
    writeConfigResultResponse(500, false, requestId, "COMMIT_FAILED", reason);
    return;
  }

  JsonDocument extras;
  extras["activeVersion"] = gActiveVersion;
  JsonObject head = extras["historyHead"].to<JsonObject>();
  writeHistoryHead(head);
  extras["requiresRestart"] = false;
  extras["usedLegacyCardsBridge"] = usedLegacyBridge;
  JsonObject extrasObj = extras.as<JsonObject>();
  writeConfigResultResponse(200, true, requestId, nullptr, "", &extrasObj);
}

void handleHttpRestoreConfig() {
  JsonDocument request;
  DeserializationError parseError =
      deserializeJson(request, gPortalServer.arg("plain"));
  if (parseError || !request.is<JsonObjectConst>()) {
    writeConfigErrorResponse(400, "INVALID_REQUEST", "invalid json");
    return;
  }

  const char* source = request["source"] | "";
  const char* restorePath = nullptr;
  if (strcmp(source, "LKG") == 0) restorePath = kLkgConfigPath;
  if (strcmp(source, "SLOT1") == 0) restorePath = kSlot1ConfigPath;
  if (strcmp(source, "SLOT2") == 0) restorePath = kSlot2ConfigPath;
  if (strcmp(source, "SLOT3") == 0) restorePath = kSlot3ConfigPath;
  if (strcmp(source, "FACTORY") == 0) restorePath = kFactoryConfigPath;
  if (restorePath == nullptr) {
    writeConfigErrorResponse(400, "VALIDATION_FAILED",
                             "invalid restore source");
    return;
  }
  if (!LittleFS.exists(restorePath)) {
    writeConfigErrorResponse(404, "NOT_FOUND", "restore source not found");
    return;
  }

  JsonDocument doc;
  if (!readJsonFromPath(restorePath, doc) || !doc.is<JsonArrayConst>()) {
    writeConfigErrorResponse(500, "RESTORE_FAILED",
                             "failed to load restore source");
    return;
  }
  JsonArrayConst cards = doc.as<JsonArrayConst>();
  String reason;
  if (!validateConfigCardsArray(cards, reason)) {
    writeConfigErrorResponse(500, "RESTORE_FAILED", reason);
    return;
  }
  if (!commitCards(cards, reason)) {
    writeConfigErrorResponse(500, "RESTORE_FAILED", reason);
    return;
  }

  JsonDocument response;
  response["ok"] = true;
  response["restoredFrom"] = source;
  response["activeVersion"] = gActiveVersion;
  response["requiresRestart"] = false;
  response["error"] = nullptr;
  String body;
  serializeJson(response, body);
  gPortalServer.send(200, "application/json", body);
}

void initPortalServer() {
  if (gPortalServerInitialized) return;
  gPortalServer.on("/", HTTP_GET, handleHttpRoot);
  gPortalServer.on("/config", HTTP_GET, handleHttpConfigPage);
  gPortalServer.on("/settings", HTTP_GET, handleHttpSettingsPage);
  gPortalServer.on("/api/snapshot", HTTP_GET, handleHttpSnapshot);
  gPortalServer.on("/api/command", HTTP_POST, handleHttpCommand);
  gPortalServer.on("/api/config/active", HTTP_GET, handleHttpGetActiveConfig);
  gPortalServer.on("/api/config/staged/save", HTTP_POST,
                   handleHttpStagedSaveConfig);
  gPortalServer.on("/api/config/staged/validate", HTTP_POST,
                   handleHttpStagedValidateConfig);
  gPortalServer.on("/api/config/commit", HTTP_POST, handleHttpCommitConfig);
  gPortalServer.on("/api/config/restore", HTTP_POST, handleHttpRestoreConfig);
  gPortalServer.on("/api/settings", HTTP_GET, handleHttpGetSettings);
  gPortalServer.on("/api/settings/wifi", HTTP_POST, handleHttpSaveSettingsWiFi);
  gPortalServer.on("/api/settings/runtime", HTTP_POST,
                   handleHttpSaveSettingsRuntime);
  gPortalServer.on("/api/settings/reconnect", HTTP_POST,
                   handleHttpReconnectWiFi);
  gPortalServer.on("/api/settings/reboot", HTTP_POST, handleHttpReboot);
  gPortalServer.on("/favicon.ico", HTTP_GET,
                   []() { gPortalServer.send(204, "text/plain", ""); });
  gPortalServer.begin();
  gPortalServerInitialized = true;
  Serial.println("Portal HTTP server started on :80");
}

void handlePortalServerLoop() { gPortalServer.handleClient(); }

void handleWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload,
                          size_t length) {
  if (type == WStype_CONNECTED) {
    IPAddress ip = gWsServer.remoteIP(clientNum);
    Serial.printf("WS client connected #%u from %u.%u.%u.%u\n", clientNum,
                  ip[0], ip[1], ip[2], ip[3]);
    return;
  }
  if (type == WStype_DISCONNECTED) {
    Serial.printf("WS client disconnected #%u\n", clientNum);
    return;
  }
  if (type != WStype_TEXT) return;

  JsonDocument doc;
  DeserializationError error =
      deserializeJson(doc, reinterpret_cast<const char*>(payload), length);
  if (error || !doc.is<JsonObject>()) {
    gWsServer.sendTXT(clientNum,
                      "{\"type\":\"command_result\",\"ok\":false,"
                      "\"error\":{\"code\":\"INVALID_REQUEST\"}}");
    return;
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  const char* typeStr = root["type"] | "";
  if (strcmp(typeStr, "command") != 0) {
    gWsServer.sendTXT(clientNum,
                      "{\"type\":\"command_result\",\"ok\":false,"
                      "\"error\":{\"code\":\"INVALID_REQUEST\"}}");
    return;
  }

  const char* requestId = root["requestId"] | "";
  bool ok = applyCommand(root);

  JsonDocument result;
  result["type"] = "command_result";
  result["schemaVersion"] = 1;
  result["requestId"] = requestId;
  result["ok"] = ok;
  if (!ok) {
    JsonObject err = result["error"].to<JsonObject>();
    err["code"] = "COMMAND_REJECTED";
  } else {
    result["error"] = nullptr;
  }
  String body;
  serializeJson(result, body);
  gWsServer.sendTXT(clientNum, body);
}

void initWebSocketServer() {
  if (gWsServerInitialized) return;
  gWsServer.begin();
  gWsServer.onEvent(handleWebSocketEvent);
  gWsServerInitialized = true;
  Serial.println("WebSocket server started on :81");
}

void handleWebSocketLoop() { gWsServer.loop(); }

void publishRuntimeSnapshotWebSocket() {
  static uint32_t lastPublishMs = 0;
  static uint32_t lastSeq = 0;

  SharedRuntimeSnapshot snapshot = {};
  copySharedRuntimeSnapshot(snapshot);
  uint32_t nowMs = millis();

  bool hasUpdate = (snapshot.seq != lastSeq);
  bool dueHeartbeat = (nowMs - lastPublishMs) >= 1000;
  if (!hasUpdate && !dueHeartbeat) return;
  if ((nowMs - lastPublishMs) < 200 && hasUpdate) return;

  JsonDocument doc;
  serializeRuntimeSnapshot(doc, nowMs);
  String payload;
  serializeJson(doc, payload);
  gWsServer.broadcastTXT(payload);

  lastPublishMs = nowMs;
  lastSeq = snapshot.seq;
}

void configureHardwarePinsSafeState() {
  for (uint8_t i = 0; i < NUM_DO; ++i) {
    pinMode(DO_Pins[i], OUTPUT);
    digitalWrite(DO_Pins[i], LOW);
  }
  for (uint8_t i = 0; i < NUM_DI; ++i) {
    pinMode(DI_Pins[i], INPUT_PULLUP);
  }
}

void bootstrapCardsFromStorage() {
  // Keep factory baseline aligned with current firmware defaults.
  {
    LogicCard factoryCards[TOTAL_CARDS];
    initializeCardArraySafeDefaults(factoryCards);
    saveCardsToPath(kFactoryConfigPath, factoryCards);
  }

  initializeAllCardsSafeDefaults();
  if (loadLogicCardsFromLittleFS()) {
    strncpy(gActiveVersion, "v1", sizeof(gActiveVersion) - 1);
    gActiveVersion[sizeof(gActiveVersion) - 1] = '\0';
    gConfigVersionCounter = 1;
    Serial.println("Loaded config from /config.json");
    return;
  }

  initializeAllCardsSafeDefaults();
  if (saveLogicCardsToLittleFS()) {
    strncpy(gActiveVersion, "v1", sizeof(gActiveVersion) - 1);
    gActiveVersion[sizeof(gActiveVersion) - 1] = '\0';
    gConfigVersionCounter = 1;
    Serial.println("Saved default config to /config.json");
  } else {
    Serial.println("Failed to save default JSON to /config.json");
  }
}

LogicCard* getCardById(uint8_t id) {
  if (id >= TOTAL_CARDS) return nullptr;
  return &logicCards[id];
}

bool isDigitalInputCard(uint8_t id) {
  return id < TOTAL_CARDS && logicCards[id].type == DigitalInput;
}

bool isDigitalOutputCard(uint8_t id) {
  return id < TOTAL_CARDS && logicCards[id].type == DigitalOutput;
}

bool isAnalogInputCard(uint8_t id) {
  return id < TOTAL_CARDS && logicCards[id].type == AnalogInput;
}

bool isSoftIOCard(uint8_t id) { return id < TOTAL_CARDS && logicCards[id].type == SoftIO; }
bool isMathCard(uint8_t id) { return id < TOTAL_CARDS && logicCards[id].type == MathCard; }
bool isRtcCard(uint8_t id) { return id < TOTAL_CARDS && logicCards[id].type == RtcCard; }

bool isInputCard(uint8_t id) {
  return isDigitalInputCard(id) || isAnalogInputCard(id);
}

void initializeRuntimeControlState() {
  strncpy(gUserSsid, kDefaultUserSsid, sizeof(gUserSsid) - 1);
  gUserSsid[sizeof(gUserSsid) - 1] = '\0';
  strncpy(gUserPassword, kDefaultUserPassword, sizeof(gUserPassword) - 1);
  gUserPassword[sizeof(gUserPassword) - 1] = '\0';

  gRunMode = RUN_NORMAL;
  gScanIntervalMs = kDefaultScanIntervalMs;
  gScanCursor = 0;
  gStepRequested = false;
  gBreakpointPaused = false;
  gTestModeActive = false;
  gGlobalOutputMask = false;
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    gCardBreakpoint[i] = false;
    gCardOutputMask[i] = false;
    gCardInputSource[i] = InputSource_Real;
    gCardForcedAIValue[i] = 0;
  }
}

bool isOutputMasked(uint8_t cardId) {
  if (!isDigitalOutputCard(cardId)) return false;
  return gGlobalOutputMask || gCardOutputMask[cardId];
}

bool setRunModeCommand(runMode mode) {
  gRunMode = mode;
  if (mode != RUN_BREAKPOINT) {
    gBreakpointPaused = false;
  }
  return true;
}

void serializeCardsToArray(const LogicCard* sourceCards, JsonArray& array) {
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    JsonObject obj = array.add<JsonObject>();
    serializeCardToJson(sourceCards[i], obj);
  }
}

const RtcScheduleChannel* findRtcScheduleByCardId(uint8_t cardId) {
  for (uint8_t i = 0; i < NUM_RTC_SCHED_CHANNELS; ++i) {
    if (gRtcScheduleChannels[i].rtcCardId == cardId) return &gRtcScheduleChannels[i];
  }
  return nullptr;
}

void writeV3ConditionClause(JsonObject& clause, uint8_t ownerId, uint8_t sourceId,
                            logicOperator op, uint32_t threshold) {
  JsonObject source = clause["source"].to<JsonObject>();
  source["cardId"] = sourceId;
  const char* field = "currentValue";
  const char* type = "NUMBER";
  const char* oper = "EQ";
  bool thresholdAsText = false;
  const char* stateText = "";
  uint32_t numericThreshold = threshold;

  switch (op) {
    case Op_AlwaysTrue:
      source["cardId"] = ownerId;
      field = "currentValue";
      type = "NUMBER";
      oper = "GTE";
      numericThreshold = 0;
      break;
    case Op_AlwaysFalse:
      source["cardId"] = ownerId;
      field = "currentValue";
      type = "NUMBER";
      oper = "GT";
      numericThreshold = UINT32_MAX;
      break;
    case Op_LogicalTrue:
      field = "logicalState";
      type = "BOOL";
      oper = "EQ";
      numericThreshold = 1;
      break;
    case Op_LogicalFalse:
      field = "logicalState";
      type = "BOOL";
      oper = "EQ";
      numericThreshold = 0;
      break;
    case Op_PhysicalOn:
      field = "physicalState";
      type = "BOOL";
      oper = "EQ";
      numericThreshold = 1;
      break;
    case Op_PhysicalOff:
      field = "physicalState";
      type = "BOOL";
      oper = "EQ";
      numericThreshold = 0;
      break;
    case Op_Triggered:
      field = "triggerFlag";
      type = "BOOL";
      oper = "EQ";
      numericThreshold = 1;
      break;
    case Op_TriggerCleared:
      field = "triggerFlag";
      type = "BOOL";
      oper = "EQ";
      numericThreshold = 0;
      break;
    case Op_GT:
      oper = "GT";
      break;
    case Op_LT:
      oper = "LT";
      break;
    case Op_EQ:
      oper = "EQ";
      break;
    case Op_NEQ:
      oper = "NEQ";
      break;
    case Op_GTE:
      oper = "GTE";
      break;
    case Op_LTE:
      oper = "LTE";
      break;
    case Op_Running:
      field = "missionState";
      type = "STATE";
      oper = "EQ";
      thresholdAsText = true;
      stateText = "ACTIVE";
      break;
    case Op_Finished:
      field = "missionState";
      type = "STATE";
      oper = "EQ";
      thresholdAsText = true;
      stateText = "FINISHED";
      break;
    case Op_Stopped:
      field = "missionState";
      type = "STATE";
      oper = "EQ";
      thresholdAsText = true;
      stateText = "IDLE";
      break;
    default:
      break;
  }

  source["field"] = field;
  source["type"] = type;
  clause["operator"] = oper;
  if (thresholdAsText) {
    clause["threshold"] = stateText;
  } else {
    clause["threshold"] = numericThreshold;
  }
}

void writeV3ConditionBlock(JsonObject& out, uint8_t ownerId, uint8_t aId,
                           logicOperator aOp, uint32_t aThreshold, uint8_t bId,
                           logicOperator bOp, uint32_t bThreshold,
                           combineMode combine) {
  if (combine == Combine_AND) {
    out["combiner"] = "AND";
  } else if (combine == Combine_OR) {
    out["combiner"] = "OR";
  } else {
    out["combiner"] = "NONE";
  }

  JsonObject clauseA = out["clauseA"].to<JsonObject>();
  writeV3ConditionClause(clauseA, ownerId, aId, aOp, aThreshold);
  if (combine != Combine_None) {
    JsonObject clauseB = out["clauseB"].to<JsonObject>();
    writeV3ConditionClause(clauseB, ownerId, bId, bOp, bThreshold);
  }
}

void serializeCardsToV3Array(const LogicCard* sourceCards, JsonArray& cards) {
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    const LogicCard& card = sourceCards[i];
    const RtcScheduleChannel* rtc = findRtcScheduleByCardId(card.id);
    const int16_t rtcYear = (rtc != nullptr) ? rtc->year : -1;
    const int8_t rtcMonth = (rtc != nullptr) ? rtc->month : -1;
    const int8_t rtcDay = (rtc != nullptr) ? rtc->day : -1;
    const int8_t rtcWeekday = (rtc != nullptr) ? rtc->weekday : -1;
    const int8_t rtcHour = (rtc != nullptr) ? rtc->hour : -1;
    const int8_t rtcMinute = (rtc != nullptr) ? rtc->minute : -1;
    V3CardConfig typed = {};
    if (!legacyToV3CardConfig(card, rtcYear, rtcMonth, rtcDay, rtcWeekday,
                              rtcHour, rtcMinute, typed)) {
      continue;
    }

    JsonObject v3 = cards.add<JsonObject>();
    v3["cardId"] = typed.cardId;
    v3["enabled"] = typed.enabled;
    v3["label"] = String("Card ") + String(typed.cardId);
    v3["faultPolicy"] = "WARN";

    JsonObject cfg = v3["config"].to<JsonObject>();
    if (typed.family == V3CardFamily::DI) {
      v3["cardType"] = "DI";
      cfg["channel"] = typed.di.channel;
      cfg["invert"] = typed.di.invert;
      cfg["debounceTime"] = typed.di.debounceTimeMs;
      if (typed.di.edgeMode == Mode_DI_Falling) {
        cfg["edgeMode"] = "FALLING";
      } else if (typed.di.edgeMode == Mode_DI_Change) {
        cfg["edgeMode"] = "CHANGE";
      } else {
        cfg["edgeMode"] = "RISING";
      }
      JsonObject set = cfg["set"].to<JsonObject>();
      writeV3ConditionBlock(
          set, typed.cardId, typed.di.set.clauseAId,
          typed.di.set.clauseAOperator, typed.di.set.clauseAThreshold,
          typed.di.set.clauseBId, typed.di.set.clauseBOperator,
          typed.di.set.clauseBThreshold, typed.di.set.combiner);
      JsonObject reset = cfg["reset"].to<JsonObject>();
      writeV3ConditionBlock(
          reset, typed.cardId, typed.di.reset.clauseAId,
          typed.di.reset.clauseAOperator, typed.di.reset.clauseAThreshold,
          typed.di.reset.clauseBId, typed.di.reset.clauseBOperator,
          typed.di.reset.clauseBThreshold, typed.di.reset.combiner);
      cfg["counterVisible"] = true;
      continue;
    }

    if (typed.family == V3CardFamily::AI) {
      v3["cardType"] = "AI";
      cfg["channel"] = typed.ai.channel;
      cfg["engineeringUnit"] = "raw";
      JsonObject inRange = cfg["inputRange"].to<JsonObject>();
      inRange["min"] = typed.ai.inputMin;
      inRange["max"] = typed.ai.inputMax;
      JsonObject clampRange = cfg["clampRange"].to<JsonObject>();
      clampRange["min"] = typed.ai.inputMin;
      clampRange["max"] = typed.ai.inputMax;
      JsonObject outRange = cfg["outputRange"].to<JsonObject>();
      outRange["min"] = typed.ai.outputMin;
      outRange["max"] = typed.ai.outputMax;
      cfg["emaAlpha"] = typed.ai.emaAlphaX100;
      continue;
    }

    if (typed.family == V3CardFamily::DO || typed.family == V3CardFamily::SIO) {
      v3["cardType"] = (typed.family == V3CardFamily::DO) ? "DO" : "SIO";
      if (typed.family == V3CardFamily::DO) cfg["channel"] = typed.dout.channel;
      cardMode mode =
          (typed.family == V3CardFamily::DO) ? typed.dout.mode : typed.sio.mode;
      if (mode == Mode_DO_Immediate) {
        cfg["mode"] = "Immediate";
      } else if (mode == Mode_DO_Gated) {
        cfg["mode"] = "Gated";
      } else {
        cfg["mode"] = "Normal";
      }
      if (typed.family == V3CardFamily::DO) {
        cfg["delayBeforeON"] = typed.dout.delayBeforeOnMs;
        cfg["onDuration"] = typed.dout.onDurationMs;
        cfg["repeatCount"] = typed.dout.repeatCount;
      } else {
        cfg["delayBeforeON"] = typed.sio.delayBeforeOnMs;
        cfg["onDuration"] = typed.sio.onDurationMs;
        cfg["repeatCount"] = typed.sio.repeatCount;
      }
      JsonObject set = cfg["set"].to<JsonObject>();
      if (typed.family == V3CardFamily::DO) {
        writeV3ConditionBlock(
            set, typed.cardId, typed.dout.set.clauseAId,
            typed.dout.set.clauseAOperator, typed.dout.set.clauseAThreshold,
            typed.dout.set.clauseBId, typed.dout.set.clauseBOperator,
            typed.dout.set.clauseBThreshold, typed.dout.set.combiner);
      } else {
        writeV3ConditionBlock(
            set, typed.cardId, typed.sio.set.clauseAId,
            typed.sio.set.clauseAOperator, typed.sio.set.clauseAThreshold,
            typed.sio.set.clauseBId, typed.sio.set.clauseBOperator,
            typed.sio.set.clauseBThreshold, typed.sio.set.combiner);
      }
      JsonObject reset = cfg["reset"].to<JsonObject>();
      if (typed.family == V3CardFamily::DO) {
        writeV3ConditionBlock(
            reset, typed.cardId, typed.dout.reset.clauseAId,
            typed.dout.reset.clauseAOperator, typed.dout.reset.clauseAThreshold,
            typed.dout.reset.clauseBId, typed.dout.reset.clauseBOperator,
            typed.dout.reset.clauseBThreshold, typed.dout.reset.combiner);
      } else {
        writeV3ConditionBlock(
            reset, typed.cardId, typed.sio.reset.clauseAId,
            typed.sio.reset.clauseAOperator, typed.sio.reset.clauseAThreshold,
            typed.sio.reset.clauseBId, typed.sio.reset.clauseBOperator,
            typed.sio.reset.clauseBThreshold, typed.sio.reset.combiner);
      }
      if (typed.family == V3CardFamily::SIO) {
        JsonObject policy = cfg["writePolicy"].to<JsonObject>();
        JsonArray roles = policy["allowedRoles"].to<JsonArray>();
        roles.add("OPERATOR");
        roles.add("ENGINEER");
        roles.add("ADMIN");
      }
      continue;
    }

    if (typed.family == V3CardFamily::MATH) {
      v3["cardType"] = "MATH";
      cfg["mode"] = "Mode_Standard_Pipeline";
      JsonObject set = cfg["set"].to<JsonObject>();
      writeV3ConditionBlock(
          set, typed.cardId, typed.math.set.clauseAId,
          typed.math.set.clauseAOperator, typed.math.set.clauseAThreshold,
          typed.math.set.clauseBId, typed.math.set.clauseBOperator,
          typed.math.set.clauseBThreshold, typed.math.set.combiner);
      JsonObject reset = cfg["reset"].to<JsonObject>();
      writeV3ConditionBlock(
          reset, typed.cardId, typed.math.reset.clauseAId,
          typed.math.reset.clauseAOperator, typed.math.reset.clauseAThreshold,
          typed.math.reset.clauseBId, typed.math.reset.clauseBOperator,
          typed.math.reset.clauseBThreshold, typed.math.reset.combiner);
      cfg["fallbackValue"] = typed.math.fallbackValue;
      JsonObject standard = cfg["standard"].to<JsonObject>();
      JsonObject inputA = standard["inputA"].to<JsonObject>();
      inputA["sourceMode"] = "CONSTANT";
      inputA["value"] = typed.math.inputA;
      JsonObject inputB = standard["inputB"].to<JsonObject>();
      inputB["sourceMode"] = "CONSTANT";
      inputB["value"] = typed.math.inputB;
      standard["operator"] = "ADD";
      standard["rateLimit"] = 0;
      standard["clampMin"] = typed.math.clampMin;
      standard["clampMax"] = typed.math.clampMax;
      standard["scaleMin"] = typed.math.clampMin;
      standard["scaleMax"] = typed.math.clampMax;
      standard["emaAlpha"] = 100;
      cfg["pid"] = nullptr;
      continue;
    }

    v3["cardType"] = "RTC";
    JsonObject schedule = cfg["schedule"].to<JsonObject>();
    if (typed.rtc.hasYear) schedule["year"] = typed.rtc.year;
    if (typed.rtc.hasMonth) schedule["month"] = typed.rtc.month;
    if (typed.rtc.hasDay) schedule["day"] = typed.rtc.day;
    if (typed.rtc.hasWeekday) schedule["weekday"] = typed.rtc.weekday;
    schedule["hour"] = typed.rtc.hour;
    schedule["minute"] = typed.rtc.minute;
    cfg["triggerDuration"] = typed.rtc.triggerDurationMs;
  }
}

void buildV3ConfigEnvelope(const LogicCard* sourceCards, JsonDocument& doc,
                           const char* configId, const char* requestId) {
  doc["apiVersion"] = kApiVersion;
  doc["schemaVersion"] = kSchemaVersion;
  doc["requestId"] = requestId;
  JsonObject config = doc["config"].to<JsonObject>();
  config["schemaVersion"] = kSchemaVersion;
  config["configId"] = configId;
  config["createdAt"] = "2026-03-01T00:00:00Z";
  JsonObject scan = config["scan"].to<JsonObject>();
  scan["intervalMs"] = gScanIntervalMs;
  scan["jitterBudgetUs"] = 500;
  scan["overrunBudgetUs"] = 1000;
  JsonArray cards = config["cards"].to<JsonArray>();
  serializeCardsToV3Array(sourceCards, cards);
  config["bindings"].to<JsonArray>();

  JsonObject wifi = config["wifi"].to<JsonObject>();
  JsonObject master = wifi["master"].to<JsonObject>();
  master["ssid"] = kMasterSsid;
  master["password"] = kMasterPassword;
  master["timeoutSec"] = MASTER_WIFI_TIMEOUT_MS / 1000;
  master["editable"] = false;
  JsonObject user = wifi["user"].to<JsonObject>();
  user["ssid"] = gUserSsid;
  user["password"] = gUserPassword;
  user["timeoutSec"] = USER_WIFI_TIMEOUT_MS / 1000;
  wifi["retryBackoffSec"] = 30;
  wifi["staOnly"] = true;
}

void initializeCardArraySafeDefaults(LogicCard* cards) {
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    initializeCardSafeDefaults(cards[i], i);
  }
}

bool deserializeCardsFromArray(JsonArrayConst array, LogicCard* outCards) {
  if (array.size() != TOTAL_CARDS) return false;
  initializeCardArraySafeDefaults(outCards);
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    JsonVariantConst item = array[i];
    if (!item.is<JsonObjectConst>()) return false;
    deserializeCardFromJson(item, outCards[i]);
  }
  return true;
}

bool validateConfigCardsArray(JsonArrayConst array, String& reason) {
  if (array.size() != TOTAL_CARDS) {
    reason = "cards size mismatch";
    return false;
  }

  auto cardTypeFromString = [](const char* s) -> logicCardType {
    return parseOrDefault(s, DigitalInput);
  };
  auto isModeAllowed = [](logicCardType type, const char* mode) -> bool {
    if (mode == nullptr) return false;
    if (type == DigitalInput) {
      return strcmp(mode, "Mode_DI_Rising") == 0 ||
             strcmp(mode, "Mode_DI_Falling") == 0 ||
             strcmp(mode, "Mode_DI_Change") == 0;
    }
    if (type == AnalogInput) return strcmp(mode, "Mode_AI_Continuous") == 0;
    if (type == DigitalOutput || type == SoftIO) {
      return strcmp(mode, "Mode_DO_Normal") == 0 ||
             strcmp(mode, "Mode_DO_Immediate") == 0 ||
             strcmp(mode, "Mode_DO_Gated") == 0;
    }
    if (type == MathCard || type == RtcCard) {
      return strcmp(mode, "Mode_None") == 0;
    }
    return false;
  };
  auto isAlwaysOp = [](const char* op) -> bool {
    return op != nullptr && (strcmp(op, "Op_AlwaysTrue") == 0 ||
                             strcmp(op, "Op_AlwaysFalse") == 0);
  };
  auto isNumericOp = [](const char* op) -> bool {
    return op != nullptr &&
           (strcmp(op, "Op_GT") == 0 || strcmp(op, "Op_LT") == 0 ||
            strcmp(op, "Op_EQ") == 0 || strcmp(op, "Op_NEQ") == 0 ||
            strcmp(op, "Op_GTE") == 0 || strcmp(op, "Op_LTE") == 0);
  };
  auto isStateOp = [](const char* op) -> bool {
    return op != nullptr && (strcmp(op, "Op_LogicalTrue") == 0 ||
                             strcmp(op, "Op_LogicalFalse") == 0 ||
                             strcmp(op, "Op_PhysicalOn") == 0 ||
                             strcmp(op, "Op_PhysicalOff") == 0);
  };
  auto isTriggerOp = [](const char* op) -> bool {
    return op != nullptr && (strcmp(op, "Op_Triggered") == 0 ||
                             strcmp(op, "Op_TriggerCleared") == 0);
  };
  auto isProcessOp = [](const char* op) -> bool {
    return op != nullptr &&
           (strcmp(op, "Op_Running") == 0 || strcmp(op, "Op_Finished") == 0 ||
            strcmp(op, "Op_Stopped") == 0);
  };
  auto isOperatorAllowedForTarget = [&](logicCardType targetType,
                                        const char* op) -> bool {
    if (isAlwaysOp(op)) return true;
    if (targetType == AnalogInput) return isNumericOp(op);
    if (targetType == DigitalInput)
      return isStateOp(op) || isTriggerOp(op) || isNumericOp(op);
    if (targetType == DigitalOutput || targetType == SoftIO) {
      return isStateOp(op) || isTriggerOp(op) || isNumericOp(op) ||
             isProcessOp(op);
    }
    if (targetType == MathCard) return isNumericOp(op);
    if (targetType == RtcCard) {
      return isStateOp(op) || isTriggerOp(op) || isNumericOp(op);
    }
    return false;
  };
  auto isNonNegativeNumber = [](JsonVariantConst v) -> bool {
    if (v.is<uint64_t>()) return true;
    if (v.is<int64_t>()) return v.as<int64_t>() >= 0;
    if (v.is<double>()) return v.as<double>() >= 0.0;
    if (v.is<float>()) return v.as<float>() >= 0.0f;
    return false;
  };
  auto ensureNonNegativeField = [&](JsonObjectConst card, const char* fieldName,
                                    const char* label) -> bool {
    JsonVariantConst v = card[fieldName];
    if (!isNonNegativeNumber(v)) {
      reason = String(label) + " must be non-negative";
      return false;
    }
    return true;
  };

  bool seenId[TOTAL_CARDS] = {};
  logicCardType typeById[TOTAL_CARDS] = {};
  bool typeKnown[TOTAL_CARDS] = {};
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    JsonVariantConst item = array[i];
    if (!item.is<JsonObjectConst>()) {
      reason = "cards[] item is not object";
      return false;
    }
    JsonObjectConst card = item.as<JsonObjectConst>();
    uint8_t id = card["id"] | 255;
    if (id >= TOTAL_CARDS) {
      reason = "card id out of range";
      return false;
    }
    if (seenId[id]) {
      reason = "duplicate card id";
      return false;
    }
    seenId[id] = true;
    typeById[id] = cardTypeFromString(card["type"] | "DigitalInput");
    typeKnown[id] = true;
    logicCardType expectedType = DigitalInput;
    if (id < DO_START) {
      expectedType = DigitalInput;
    } else if (id < AI_START) {
      expectedType = DigitalOutput;
    } else if (id < SIO_START) {
      expectedType = AnalogInput;
    } else if (id < MATH_START) {
      expectedType = SoftIO;
    } else if (id < RTC_START) {
      expectedType = MathCard;
    } else {
      expectedType = RtcCard;
    }
    if (typeById[id] != expectedType) {
      reason = "card type does not match fixed family slot";
      return false;
    }

    uint8_t setAId = card["setA_ID"] | 255;
    uint8_t setBId = card["setB_ID"] | 255;
    uint8_t resetAId = card["resetA_ID"] | 255;
    uint8_t resetBId = card["resetB_ID"] | 255;
    if (setAId >= TOTAL_CARDS || setBId >= TOTAL_CARDS ||
        resetAId >= TOTAL_CARDS || resetBId >= TOTAL_CARDS) {
      reason = "set/reset reference id out of range";
      return false;
    }
  }

  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    JsonObjectConst card = array[i].as<JsonObjectConst>();
    uint8_t id = card["id"] | 255;
    if (id >= TOTAL_CARDS || !typeKnown[id]) {
      reason = "card id/type map error";
      return false;
    }

    const char* mode = card["mode"] | "";
    if (!isModeAllowed(typeById[id], mode)) {
      reason = "mode not valid for card type (id=" + String(id) +
               ", type=" + String(toString(typeById[id])) +
               ", mode=" + String(mode) + ")";
      return false;
    }

    if (!ensureNonNegativeField(card, "hwPin", "hwPin")) return false;
    if (!ensureNonNegativeField(card, "setting1", "setting1")) return false;
    if (!ensureNonNegativeField(card, "setting2", "setting2")) return false;
    if (!ensureNonNegativeField(card, "setting3", "setting3")) return false;
    if (!ensureNonNegativeField(card, "startOnMs", "startOnMs")) return false;
    if (!ensureNonNegativeField(card, "startOffMs", "startOffMs")) return false;
    if (!ensureNonNegativeField(card, "setA_Threshold", "setA_Threshold"))
      return false;
    if (!ensureNonNegativeField(card, "setB_Threshold", "setB_Threshold"))
      return false;
    if (!ensureNonNegativeField(card, "resetA_Threshold", "resetA_Threshold"))
      return false;
    if (!ensureNonNegativeField(card, "resetB_Threshold", "resetB_Threshold"))
      return false;

    if (typeById[id] == AnalogInput) {
      const double alpha = card["setting3"] | 0.0;
      if (alpha < 0.0 || alpha > 1.0) {
        reason = "AI setting3 alpha out of range (0..1)";
        return false;
      }
    }

    if (typeById[id] == RtcCard) {
      const char* setAOp = card["setA_Operator"] | "";
      const char* setBOp = card["setB_Operator"] | "";
      const char* resetAOp = card["resetA_Operator"] | "";
      const char* resetBOp = card["resetB_Operator"] | "";
      const char* setCombine = card["setCombine"] | "";
      const char* resetCombine = card["resetCombine"] | "";
      const bool rtcSetResetDisabled =
          strcmp(setAOp, "Op_AlwaysFalse") == 0 &&
          strcmp(setBOp, "Op_AlwaysFalse") == 0 &&
          strcmp(resetAOp, "Op_AlwaysFalse") == 0 &&
          strcmp(resetBOp, "Op_AlwaysFalse") == 0 &&
          strcmp(setCombine, "Combine_None") == 0 &&
          strcmp(resetCombine, "Combine_None") == 0;
      if (!rtcSetResetDisabled) {
        reason = "RTC set/reset is unsupported";
        return false;
      }
    }

    uint8_t setAId = card["setA_ID"] | 255;
    uint8_t setBId = card["setB_ID"] | 255;
    uint8_t resetAId = card["resetA_ID"] | 255;
    uint8_t resetBId = card["resetB_ID"] | 255;
    const char* setAOp = card["setA_Operator"] | "";
    const char* setBOp = card["setB_Operator"] | "";
    const char* resetAOp = card["resetA_Operator"] | "";
    const char* resetBOp = card["resetB_Operator"] | "";

    if (!isOperatorAllowedForTarget(typeById[setAId], setAOp) ||
        !isOperatorAllowedForTarget(typeById[setBId], setBOp) ||
        !isOperatorAllowedForTarget(typeById[resetAId], resetAOp) ||
        !isOperatorAllowedForTarget(typeById[resetBId], resetBOp)) {
      reason = "operator not valid for referenced card type";
      return false;
    }
  }

  reason = "";
  return true;
}

bool writeJsonToPath(const char* path, JsonDocument& doc) {
  File file = LittleFS.open(path, "w");
  if (!file) return false;
  size_t written = serializeJson(doc, file);
  file.close();
  return written > 0;
}

bool readJsonFromPath(const char* path, JsonDocument& doc) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  return !error;
}

bool loadPortalSettingsFromLittleFS() {
  if (!LittleFS.exists(kPortalSettingsPath)) return false;
  JsonDocument doc;
  if (!readJsonFromPath(kPortalSettingsPath, doc)) return false;
  if (!doc.is<JsonObjectConst>()) return false;
  JsonObjectConst root = doc.as<JsonObjectConst>();

  const char* userSsid = root["userSsid"] | "";
  const char* userPassword = root["userPassword"] | "";
  const uint32_t scanIntervalMs =
      root["scanIntervalMs"] | static_cast<uint32_t>(kDefaultScanIntervalMs);

  if (strlen(userSsid) > 0 && strlen(userSsid) <= 32) {
    strncpy(gUserSsid, userSsid, sizeof(gUserSsid) - 1);
    gUserSsid[sizeof(gUserSsid) - 1] = '\0';
  }
  if (strlen(userPassword) <= 64) {
    strncpy(gUserPassword, userPassword, sizeof(gUserPassword) - 1);
    gUserPassword[sizeof(gUserPassword) - 1] = '\0';
  }
  if (scanIntervalMs >= kMinScanIntervalMs &&
      scanIntervalMs <= kMaxScanIntervalMs) {
    gScanIntervalMs = scanIntervalMs;
  }
  return true;
}

bool savePortalSettingsToLittleFS() {
  JsonDocument doc;
  doc["userSsid"] = gUserSsid;
  doc["userPassword"] = gUserPassword;
  doc["scanIntervalMs"] = gScanIntervalMs;
  return writeJsonToPath(kPortalSettingsPath, doc);
}

bool saveCardsToPath(const char* path, const LogicCard* sourceCards) {
  JsonDocument doc;
  buildV3ConfigEnvelope(sourceCards, doc, gActiveVersion, "");
  return writeJsonToPath(path, doc);
}

bool loadCardsFromPath(const char* path, LogicCard* outCards) {
  JsonDocument doc;
  if (!readJsonFromPath(path, doc)) return false;
  if (doc.is<JsonArrayConst>()) {
    return deserializeCardsFromArray(doc.as<JsonArrayConst>(), outCards);
  }
  if (!doc.is<JsonObjectConst>()) return false;
  JsonDocument normalized;
  JsonArrayConst cards;
  String reason;
  const char* errorCode = "VALIDATION_FAILED";
  bool usedLegacyBridge = false;
  if (!normalizeConfigRequest(doc.as<JsonObjectConst>(), normalized, cards, reason,
                              errorCode, usedLegacyBridge)) {
    return false;
  }
  return deserializeCardsFromArray(cards, outCards);
}

bool copyFileIfExists(const char* srcPath, const char* dstPath) {
  if (!LittleFS.exists(srcPath)) return true;
  File src = LittleFS.open(srcPath, "r");
  if (!src) return false;
  File dst = LittleFS.open(dstPath, "w");
  if (!dst) {
    src.close();
    return false;
  }
  uint8_t buffer[256];
  while (true) {
    size_t n = src.read(buffer, sizeof(buffer));
    if (n == 0) break;
    if (dst.write(buffer, n) != n) {
      src.close();
      dst.close();
      return false;
    }
  }
  src.close();
  dst.close();
  return true;
}

void formatVersion(char* out, size_t outSize, uint32_t version) {
  snprintf(out, outSize, "v%lu", static_cast<unsigned long>(version));
}

bool pauseKernelForConfigApply(uint32_t timeoutMs) {
  gKernelPauseRequested = true;
  uint32_t start = millis();
  while (!gKernelPaused && (millis() - start) < timeoutMs) {
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  return gKernelPaused;
}

void resumeKernelAfterConfigApply() { gKernelPauseRequested = false; }

void rotateHistoryVersions() {
  strncpy(gSlot3Version, gSlot2Version, sizeof(gSlot3Version) - 1);
  gSlot3Version[sizeof(gSlot3Version) - 1] = '\0';
  strncpy(gSlot2Version, gSlot1Version, sizeof(gSlot2Version) - 1);
  gSlot2Version[sizeof(gSlot2Version) - 1] = '\0';
  strncpy(gSlot1Version, gLkgVersion, sizeof(gSlot1Version) - 1);
  gSlot1Version[sizeof(gSlot1Version) - 1] = '\0';
  strncpy(gLkgVersion, gActiveVersion, sizeof(gLkgVersion) - 1);
  gLkgVersion[sizeof(gLkgVersion) - 1] = '\0';
}

bool applyCardsAsActiveConfig(const LogicCard* newCards) {
  if (!pauseKernelForConfigApply(1000)) {
    resumeKernelAfterConfigApply();
    return false;
  }
  memcpy(logicCards, newCards, sizeof(logicCards));
  memset(gPrevSetCondition, 0, sizeof(gPrevSetCondition));
  memset(gPrevDISample, 0, sizeof(gPrevDISample));
  memset(gPrevDIPrimed, 0, sizeof(gPrevDIPrimed));
  updateSharedRuntimeSnapshot(millis(), false);
  resumeKernelAfterConfigApply();
  return true;
}

logicCardType expectedTypeForCardId(uint8_t id) {
  if (id < DO_START) return DigitalInput;
  if (id < AI_START) return DigitalOutput;
  if (id < SIO_START) return AnalogInput;
  if (id < MATH_START) return SoftIO;
  if (id < RTC_START) return MathCard;
  return RtcCard;
}

bool mapV3ModeToLegacy(logicCardType type, const char* mode, cardMode& outMode) {
  if (type == DigitalInput) {
    if (strcmp(mode, "RISING") == 0) return (outMode = Mode_DI_Rising), true;
    if (strcmp(mode, "FALLING") == 0) return (outMode = Mode_DI_Falling), true;
    if (strcmp(mode, "CHANGE") == 0) return (outMode = Mode_DI_Change), true;
    return false;
  }
  if (type == DigitalOutput || type == SoftIO) {
    if (strcmp(mode, "Normal") == 0) return (outMode = Mode_DO_Normal), true;
    if (strcmp(mode, "Immediate") == 0) return (outMode = Mode_DO_Immediate), true;
    if (strcmp(mode, "Gated") == 0) return (outMode = Mode_DO_Gated), true;
    return false;
  }
  if (type == AnalogInput) return (outMode = Mode_AI_Continuous), true;
  return (outMode = Mode_None), true;
}

bool parseThresholdAsUInt(JsonVariantConst threshold, uint32_t& out) {
  if (threshold.is<uint64_t>()) {
    uint64_t v = threshold.as<uint64_t>();
    if (v > UINT32_MAX) return false;
    out = static_cast<uint32_t>(v);
    return true;
  }
  if (threshold.is<int64_t>()) {
    int64_t v = threshold.as<int64_t>();
    if (v < 0 || v > static_cast<int64_t>(UINT32_MAX)) return false;
    out = static_cast<uint32_t>(v);
    return true;
  }
  if (threshold.is<bool>()) {
    out = threshold.as<bool>() ? 1U : 0U;
    return true;
  }
  return false;
}

logicOperator mapV3ClauseToLegacyOperator(logicCardType sourceType,
                                          const char* field, const char* op,
                                          JsonVariantConst threshold,
                                          uint32_t& ioThreshold, bool& ok) {
  ok = true;
  if (!isV3FieldAllowedForSourceType(sourceType, field) ||
      !isV3OperatorAllowedForField(field, op)) {
    ok = false;
    return Op_AlwaysFalse;
  }

  if (strcmp(field, "missionState") == 0) {
    const char* stateName = threshold.as<const char*>();
    if (strcmp(op, "EQ") != 0 || stateName == nullptr) {
      ok = false;
      return Op_AlwaysFalse;
    }
    ioThreshold = 0;
    if (strcmp(stateName, "IDLE") == 0) return Op_Stopped;
    if (strcmp(stateName, "ACTIVE") == 0) return Op_Running;
    if (strcmp(stateName, "FINISHED") == 0) return Op_Finished;
    ok = false;
    return Op_AlwaysFalse;
  }

  uint32_t numeric = 0;
  if (!parseThresholdAsUInt(threshold, numeric)) {
    ok = false;
    return Op_AlwaysFalse;
  }
  ioThreshold = numeric;

  if (strcmp(field, "logicalState") == 0 || strcmp(field, "physicalState") == 0) {
    if (strcmp(op, "EQ") == 0) {
      if (strcmp(field, "logicalState") == 0) {
        return (numeric != 0) ? Op_LogicalTrue : Op_LogicalFalse;
      }
      return (numeric != 0) ? Op_PhysicalOn : Op_PhysicalOff;
    }
    if (strcmp(op, "NEQ") == 0) {
      if (strcmp(field, "logicalState") == 0) {
        return (numeric != 0) ? Op_LogicalFalse : Op_LogicalTrue;
      }
      return (numeric != 0) ? Op_PhysicalOff : Op_PhysicalOn;
    }
  }

  if (strcmp(field, "triggerFlag") == 0) {
    if (strcmp(op, "EQ") == 0) return (numeric != 0) ? Op_Triggered : Op_TriggerCleared;
    if (strcmp(op, "NEQ") == 0) return (numeric != 0) ? Op_TriggerCleared : Op_Triggered;
  }

  if (strcmp(op, "GT") == 0) return Op_GT;
  if (strcmp(op, "GTE") == 0) return Op_GTE;
  if (strcmp(op, "LT") == 0) return Op_LT;
  if (strcmp(op, "LTE") == 0) return Op_LTE;
  if (strcmp(op, "EQ") == 0) return Op_EQ;
  if (strcmp(op, "NEQ") == 0) return Op_NEQ;
  ok = false;
  return Op_AlwaysFalse;
}

bool mapV3ConditionBlock(JsonObjectConst block, uint8_t& outAId,
                         logicOperator& outAOp, uint32_t& outAThreshold,
                         uint8_t& outBId, logicOperator& outBOp,
                         uint32_t& outBThreshold, combineMode& outCombine,
                         String& reason,
                         const logicCardType* sourceTypeById) {
  const char* combiner = block["combiner"] | "NONE";
  if (strcmp(combiner, "NONE") == 0) {
    outCombine = Combine_None;
  } else if (strcmp(combiner, "AND") == 0) {
    outCombine = Combine_AND;
  } else if (strcmp(combiner, "OR") == 0) {
    outCombine = Combine_OR;
  } else {
    reason = "invalid condition combiner";
    return false;
  }

  if (!block["clauseA"].is<JsonObjectConst>()) {
    reason = "missing clauseA";
    return false;
  }
  JsonObjectConst a = block["clauseA"].as<JsonObjectConst>();
  if (!a["source"].is<JsonObjectConst>()) {
    reason = "clauseA.source missing";
    return false;
  }
  JsonObjectConst aSource = a["source"].as<JsonObjectConst>();
  outAId = aSource["cardId"] | 255;
  if (outAId >= TOTAL_CARDS) {
    reason = "clauseA source cardId out of range";
    return false;
  }
  const char* aField = aSource["field"] | "currentValue";
  const char* aOperator = a["operator"] | "";
  bool aOk = false;
  outAOp = mapV3ClauseToLegacyOperator(sourceTypeById[outAId], aField, aOperator, a["threshold"],
                                       outAThreshold, aOk);
  if (!aOk) {
    reason = "invalid clauseA field/operator/threshold for source card type";
    return false;
  }

  outBId = outAId;
  outBOp = Op_AlwaysFalse;
  outBThreshold = 0;
  if (outCombine == Combine_None) return true;

  if (!block["clauseB"].is<JsonObjectConst>()) {
    reason = "missing clauseB";
    return false;
  }
  JsonObjectConst b = block["clauseB"].as<JsonObjectConst>();
  if (!b["source"].is<JsonObjectConst>()) {
    reason = "clauseB.source missing";
    return false;
  }
  JsonObjectConst bSource = b["source"].as<JsonObjectConst>();
  outBId = bSource["cardId"] | 255;
  if (outBId >= TOTAL_CARDS) {
    reason = "clauseB source cardId out of range";
    return false;
  }
  const char* bField = bSource["field"] | "currentValue";
  const char* bOperator = b["operator"] | "";
  bool bOk = false;
  outBOp = mapV3ClauseToLegacyOperator(sourceTypeById[outBId], bField, bOperator, b["threshold"],
                                       outBThreshold, bOk);
  if (!bOk) {
    reason = "invalid clauseB field/operator/threshold for source card type";
    return false;
  }
  return true;
}

bool applyV3CardToLegacy(LogicCard& target, JsonObjectConst v3Card, String& reason,
                         const logicCardType* sourceTypeById) {
  const uint8_t cardId = v3Card["cardId"] | 255;
  const char* cardType = v3Card["cardType"] | "";
  const logicCardType expectedType = expectedTypeForCardId(cardId);
  bool typeMatches = false;
  if (expectedType == DigitalInput) typeMatches = strcmp(cardType, "DI") == 0;
  if (expectedType == DigitalOutput) typeMatches = strcmp(cardType, "DO") == 0;
  if (expectedType == AnalogInput) typeMatches = strcmp(cardType, "AI") == 0;
  if (expectedType == SoftIO) typeMatches = strcmp(cardType, "SIO") == 0;
  if (expectedType == MathCard) typeMatches = strcmp(cardType, "MATH") == 0;
  if (expectedType == RtcCard) typeMatches = strcmp(cardType, "RTC") == 0;
  if (!typeMatches) {
    reason = "cardType does not match cardId family slot";
    return false;
  }

  JsonObjectConst cfg = v3Card["config"].as<JsonObjectConst>();
  if (expectedType == DigitalInput) {
    target.invert = cfg["invert"] | false;
    target.setting1 = cfg["debounceTime"] | target.setting1;
    const char* edgeMode = cfg["edgeMode"] | "RISING";
    cardMode diMode = target.mode;
    if (!mapV3ModeToLegacy(expectedType, edgeMode, diMode)) {
      reason = "invalid DI edgeMode";
      return false;
    }
    target.mode = diMode;
    if (cfg["set"].is<JsonObjectConst>()) {
      if (!mapV3ConditionBlock(cfg["set"].as<JsonObjectConst>(), target.setA_ID,
                               target.setA_Operator, target.setA_Threshold,
                               target.setB_ID, target.setB_Operator,
                               target.setB_Threshold, target.setCombine,
                               reason, sourceTypeById)) {
        return false;
      }
    }
    if (cfg["reset"].is<JsonObjectConst>()) {
      if (!mapV3ConditionBlock(cfg["reset"].as<JsonObjectConst>(), target.resetA_ID,
                               target.resetA_Operator, target.resetA_Threshold,
                               target.resetB_ID, target.resetB_Operator,
                               target.resetB_Threshold, target.resetCombine,
                               reason, sourceTypeById)) {
        return false;
      }
    }
    return true;
  }

  if (expectedType == AnalogInput) {
    JsonObjectConst inputRange = cfg["inputRange"].as<JsonObjectConst>();
    JsonObjectConst outputRange = cfg["outputRange"].as<JsonObjectConst>();
    target.setting1 = inputRange["min"] | target.setting1;
    target.setting2 = inputRange["max"] | target.setting2;
    const uint32_t emaAlphaX100 = cfg["emaAlpha"] | 100U;
    if (emaAlphaX100 > 100U) {
      reason = "AI emaAlpha out of range";
      return false;
    }
    target.setting3 = emaAlphaX100 * 10U;
    target.startOnMs = outputRange["min"] | target.startOnMs;
    target.startOffMs = outputRange["max"] | target.startOffMs;
    target.mode = Mode_AI_Continuous;
    return true;
  }

  if (expectedType == DigitalOutput || expectedType == SoftIO) {
    const char* mode = cfg["mode"] | "Normal";
    cardMode outMode = target.mode;
    if (!mapV3ModeToLegacy(expectedType, mode, outMode)) {
      reason = "invalid DO/SIO mode";
      return false;
    }
    target.mode = outMode;
    target.setting1 = cfg["delayBeforeON"] | target.setting1;
    target.setting2 = cfg["onDuration"] | target.setting2;
    target.setting3 = cfg["repeatCount"] | target.setting3;
    if (!cfg["set"].is<JsonObjectConst>() || !cfg["reset"].is<JsonObjectConst>()) {
      reason = "DO/SIO require set and reset blocks";
      return false;
    }
    if (!mapV3ConditionBlock(cfg["set"].as<JsonObjectConst>(), target.setA_ID,
                             target.setA_Operator, target.setA_Threshold,
                             target.setB_ID, target.setB_Operator,
                             target.setB_Threshold, target.setCombine, reason,
                             sourceTypeById)) {
      return false;
    }
    if (!mapV3ConditionBlock(cfg["reset"].as<JsonObjectConst>(), target.resetA_ID,
                             target.resetA_Operator, target.resetA_Threshold,
                             target.resetB_ID, target.resetB_Operator,
                             target.resetB_Threshold, target.resetCombine,
                             reason, sourceTypeById)) {
      return false;
    }
    return true;
  }

  if (expectedType == MathCard) {
    target.setting3 = cfg["fallbackValue"] | target.setting3;
    JsonObjectConst standard = cfg["standard"].as<JsonObjectConst>();
    JsonObjectConst inputA = standard["inputA"].as<JsonObjectConst>();
    JsonObjectConst inputB = standard["inputB"].as<JsonObjectConst>();
    target.setting1 = inputA["value"] | target.setting1;
    target.setting2 = inputB["value"] | target.setting2;
    target.startOnMs = standard["clampMin"] | target.startOnMs;
    target.startOffMs = standard["clampMax"] | target.startOffMs;
    target.mode = Mode_None;
    if (cfg["set"].is<JsonObjectConst>()) {
      if (!mapV3ConditionBlock(cfg["set"].as<JsonObjectConst>(), target.setA_ID,
                               target.setA_Operator, target.setA_Threshold,
                               target.setB_ID, target.setB_Operator,
                               target.setB_Threshold, target.setCombine,
                               reason, sourceTypeById)) {
        return false;
      }
    }
    if (cfg["reset"].is<JsonObjectConst>()) {
      if (!mapV3ConditionBlock(cfg["reset"].as<JsonObjectConst>(), target.resetA_ID,
                               target.resetA_Operator, target.resetA_Threshold,
                               target.resetB_ID, target.resetB_Operator,
                               target.resetB_Threshold, target.resetCombine,
                               reason, sourceTypeById)) {
        return false;
      }
    }
    return true;
  }

  if (expectedType == RtcCard) {
    target.mode = Mode_None;
    target.setA_Operator = Op_AlwaysFalse;
    target.setB_Operator = Op_AlwaysFalse;
    target.resetA_Operator = Op_AlwaysFalse;
    target.resetB_Operator = Op_AlwaysFalse;
    target.setCombine = Combine_None;
    target.resetCombine = Combine_None;
    JsonObjectConst schedule = cfg["schedule"].as<JsonObjectConst>();
    const int slot = cardId - RTC_START;
    if (slot >= 0 && slot < NUM_RTC_SCHED_CHANNELS) {
      gRtcScheduleChannels[slot].enabled = true;
      gRtcScheduleChannels[slot].year = schedule["year"] | -1;
      gRtcScheduleChannels[slot].month = schedule["month"] | -1;
      gRtcScheduleChannels[slot].day = schedule["day"] | -1;
      gRtcScheduleChannels[slot].weekday = schedule["weekday"] | -1;
      gRtcScheduleChannels[slot].hour = schedule["hour"] | -1;
      gRtcScheduleChannels[slot].minute = schedule["minute"] | -1;
      gRtcScheduleChannels[slot].rtcCardId = cardId;
    }
    target.setting1 = cfg["triggerDuration"] | target.setting1;
    return true;
  }

  reason = "unsupported card type";
  return false;
}

bool buildLegacyCardsFromV3Cards(JsonArrayConst v3Cards, JsonDocument& doc,
                                 JsonArrayConst& outCards, String& reason) {
  LogicCard mapped[TOTAL_CARDS];
  initializeCardArraySafeDefaults(mapped);
  bool seen[TOTAL_CARDS] = {};
  logicCardType sourceTypeById[TOTAL_CARDS] = {};
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    sourceTypeById[i] = expectedTypeForCardId(i);
  }

  for (JsonVariantConst v : v3Cards) {
    if (!v.is<JsonObjectConst>()) {
      reason = "cards[] item is not object";
      return false;
    }
    JsonObjectConst card = v.as<JsonObjectConst>();
    const uint8_t cardId = card["cardId"] | 255;
    if (cardId >= TOTAL_CARDS) {
      reason = "cardId out of range";
      return false;
    }
    logicCardType parsedType = DigitalInput;
    if (!parseV3CardTypeToken(card["cardType"] | "", parsedType)) {
      reason = "invalid cardType";
      return false;
    }
    const logicCardType expectedType = expectedTypeForCardId(cardId);
    if (parsedType != expectedType) {
      reason = "cardType does not match cardId family slot";
      return false;
    }
    sourceTypeById[cardId] = parsedType;
  }

  for (JsonVariantConst v : v3Cards) {
    if (!v.is<JsonObjectConst>()) {
      reason = "cards[] item is not object";
      return false;
    }
    JsonObjectConst card = v.as<JsonObjectConst>();
    const uint8_t cardId = card["cardId"] | 255;
    if (cardId >= TOTAL_CARDS) {
      reason = "cardId out of range";
      return false;
    }
    if (seen[cardId]) {
      reason = "duplicate cardId";
      return false;
    }
    seen[cardId] = true;
    if (!card["config"].is<JsonObjectConst>()) {
      reason = "missing card config";
      return false;
    }
    if (!applyV3CardToLegacy(mapped[cardId], card, reason, sourceTypeById))
      return false;
  }

  JsonObject normalized = doc["config"].to<JsonObject>();
  JsonArray cards = normalized["cards"].to<JsonArray>();
  serializeCardsToArray(mapped, cards);
  outCards = cards;
  return true;
}

bool hasLegacyCardsShape(JsonArrayConst cards) {
  for (JsonVariantConst v : cards) {
    if (!v.is<JsonObjectConst>()) continue;
    JsonObjectConst o = v.as<JsonObjectConst>();
    if (o["id"].is<uint64_t>() || o["type"].is<const char*>()) return true;
    break;
  }
  return false;
}

bool hasV3CardsShape(JsonArrayConst cards) {
  for (JsonVariantConst v : cards) {
    if (!v.is<JsonObjectConst>()) continue;
    JsonObjectConst o = v.as<JsonObjectConst>();
    if (o["cardId"].is<uint64_t>() || o["cardType"].is<const char*>()) return true;
    break;
  }
  return false;
}

bool normalizeConfigRequest(JsonObjectConst root, JsonDocument& normalizedDoc,
                            JsonArrayConst& outCards, String& reason,
                            const char*& outErrorCode,
                            bool& usedLegacyBridge) {
  usedLegacyBridge = false;
  outErrorCode = "VALIDATION_FAILED";
  const char* requestId = root["requestId"] | "";
  const char* apiVersion = root["apiVersion"].as<const char*>();
  const char* schemaVersion = root["schemaVersion"].as<const char*>();

  if (apiVersion != nullptr && strcmp(apiVersion, kApiVersion) != 0) {
    reason = "unsupported apiVersion";
    outErrorCode = "UNSUPPORTED_API_VERSION";
    return false;
  }
  if (schemaVersion != nullptr && strcmp(schemaVersion, kSchemaVersion) != 0) {
    reason = "unsupported schemaVersion";
    outErrorCode = "UNSUPPORTED_SCHEMA_VERSION";
    return false;
  }
  if (!root["config"].is<JsonObjectConst>()) {
    reason = "missing config object";
    outErrorCode = "INVALID_REQUEST";
    return false;
  }
  JsonObjectConst config = root["config"].as<JsonObjectConst>();
  if (!config["cards"].is<JsonArrayConst>()) {
    reason = "missing config.cards array";
    outErrorCode = "INVALID_REQUEST";
    return false;
  }
  JsonArrayConst inputCards = config["cards"].as<JsonArrayConst>();

  normalizedDoc["requestId"] = requestId;
  normalizedDoc["apiVersion"] = kApiVersion;
  normalizedDoc["schemaVersion"] = kSchemaVersion;

  if (hasLegacyCardsShape(inputCards)) {
    usedLegacyBridge = true;
    Serial.println("Config bridge: accepted legacy cards payload");
    JsonObject outConfig = normalizedDoc["config"].to<JsonObject>();
    JsonArray out = outConfig["cards"].to<JsonArray>();
    for (JsonVariantConst v : inputCards) {
      out.add(v);
    }
    outCards = out;
  } else if (hasV3CardsShape(inputCards)) {
    if (!buildLegacyCardsFromV3Cards(inputCards, normalizedDoc, outCards, reason)) {
      return false;
    }
  } else {
    reason = "cards payload shape is unsupported";
    outErrorCode = "INVALID_REQUEST";
    return false;
  }

  if (!validateConfigCardsArray(outCards, reason)) {
    outErrorCode = "VALIDATION_FAILED";
    return false;
  }

  JsonObject meta = normalizedDoc["bridge"].to<JsonObject>();
  meta["usedLegacyCardsBridge"] = usedLegacyBridge;
  meta["bridgeVersion"] = 1;
  return true;
}

bool extractConfigCardsFromRequest(JsonObjectConst root,
                                   JsonArrayConst& outCards, String& reason) {
  if (!root["config"].is<JsonObjectConst>()) {
    reason = "missing config object";
    return false;
  }
  JsonObjectConst config = root["config"].as<JsonObjectConst>();
  if (!config["cards"].is<JsonArrayConst>()) {
    reason = "missing config.cards array";
    return false;
  }
  outCards = config["cards"].as<JsonArrayConst>();
  if (!validateConfigCardsArray(outCards, reason)) return false;
  reason = "";
  return true;
}

void writeConfigErrorResponse(int statusCode, const char* code,
                              const String& message) {
  JsonDocument doc;
  doc["ok"] = false;
  JsonObject error = doc["error"].to<JsonObject>();
  error["code"] = code;
  error["message"] = message;
  String body;
  serializeJson(doc, body);
  gPortalServer.send(statusCode, "application/json", body);
}

void writeConfigResultResponse(int statusCode, bool ok, const char* requestId,
                               const char* errorCode, const String& message,
                               JsonObject* extra) {
  JsonDocument doc;
  doc["apiVersion"] = kApiVersion;
  doc["requestId"] = requestId;
  doc["status"] = ok ? "SUCCESS" : "FAILURE";
  if (!ok) {
    doc["errorCode"] = errorCode;
    doc["message"] = message;
  } else {
    doc["errorCode"] = nullptr;
    doc["message"] = nullptr;
  }
  if (extra != nullptr) {
    for (JsonPair kv : *extra) {
      doc[kv.key()] = kv.value();
    }
  }
  String body;
  serializeJson(doc, body);
  gPortalServer.send(statusCode, "application/json", body);
}

bool requestStepCommand() {
  gStepRequested = true;
  gBreakpointPaused = false;
  gRunMode = RUN_STEP;
  return true;
}

bool setBreakpointCommand(uint8_t cardId, bool enabled) {
  if (cardId >= TOTAL_CARDS) return false;
  gCardBreakpoint[cardId] = enabled;
  if (!enabled) gBreakpointPaused = false;
  return true;
}

bool setTestModeCommand(bool active) {
  gTestModeActive = active;
  if (!active) {
    for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
      gCardInputSource[i] = InputSource_Real;
      gCardOutputMask[i] = false;
      gCardForcedAIValue[i] = 0;
    }
    gGlobalOutputMask = false;
  }
  return true;
}

bool setOutputMaskCommand(uint8_t cardId, bool masked) {
  if (cardId >= TOTAL_CARDS) return false;
  if (!isDigitalOutputCard(cardId)) return false;
  gCardOutputMask[cardId] = masked;
  return true;
}

bool setGlobalOutputMaskCommand(bool masked) {
  gGlobalOutputMask = masked;
  return true;
}

bool setInputForceCommand(uint8_t cardId, inputSourceMode mode,
                          uint32_t forcedValue) {
  if (cardId >= TOTAL_CARDS) return false;
  if (!isInputCard(cardId)) return false;

  if (isDigitalInputCard(cardId)) {
    if (mode == InputSource_ForcedValue) return false;
  } else if (isAnalogInputCard(cardId)) {
    if (mode == InputSource_ForcedHigh || mode == InputSource_ForcedLow) {
      return false;
    }
  }

  gCardInputSource[cardId] = mode;
  if (mode == InputSource_ForcedValue) gCardForcedAIValue[cardId] = forcedValue;
  if (mode == InputSource_Real) gCardForcedAIValue[cardId] = 0;
  return true;
}

bool setRtcCardStateCommand(uint8_t cardId, bool state) {
  if (cardId >= TOTAL_CARDS) return false;
  LogicCard& card = logicCards[cardId];
  if (card.type != RtcCard) return false;

  card.logicalState = state;
  card.physicalState = state;
  card.triggerFlag = state;  // one-minute assertion edge for condition logic
  card.currentValue = state ? 1 : 0;
  if (!state) {
    card.triggerFlag = false;
  }
  return true;
}

bool enqueueKernelCommand(const KernelCommand& command) {
  if (gKernelCommandQueue == nullptr) return false;
  KernelCommand commandToQueue = command;
  commandToQueue.enqueuedUs = micros();
  bool queued = xQueueSend(gKernelCommandQueue, &commandToQueue, 0) == pdTRUE;
  gKernelQueueDepth =
      static_cast<uint16_t>(uxQueueMessagesWaiting(gKernelCommandQueue));
  if (gKernelQueueDepth > gKernelQueueHighWaterMark) {
    gKernelQueueHighWaterMark = gKernelQueueDepth;
  }
  return queued;
}

bool applyKernelCommand(const KernelCommand& command) {
  switch (command.type) {
    case KernelCmd_SetRunMode:
      return setRunModeCommand(command.mode);
    case KernelCmd_StepOnce:
      return requestStepCommand();
    case KernelCmd_SetBreakpoint:
      return setBreakpointCommand(command.cardId, command.flag);
    case KernelCmd_SetTestMode:
      return setTestModeCommand(command.flag);
    case KernelCmd_SetInputForce:
      return setInputForceCommand(command.cardId, command.inputMode,
                                  command.value);
    case KernelCmd_SetOutputMask:
      return setOutputMaskCommand(command.cardId, command.flag);
    case KernelCmd_SetOutputMaskGlobal:
      return setGlobalOutputMaskCommand(command.flag);
    case KernelCmd_SetRtcCardState:
      return setRtcCardStateCommand(command.cardId, command.flag);
    default:
      return false;
  }
}

void processKernelCommandQueue() {
  if (gKernelCommandQueue == nullptr) return;
  KernelCommand command = {};
  while (xQueueReceive(gKernelCommandQueue, &command, 0) == pdTRUE) {
    uint32_t nowUs = micros();
    uint32_t latencyUs = nowUs - command.enqueuedUs;
    gCommandLatencyLastUs = latencyUs;
    if (latencyUs > gCommandLatencyMaxUs) gCommandLatencyMaxUs = latencyUs;
    applyKernelCommand(command);
  }
  gKernelQueueDepth =
      static_cast<uint16_t>(uxQueueMessagesWaiting(gKernelCommandQueue));
  if (gKernelQueueDepth > gKernelQueueHighWaterMark) {
    gKernelQueueHighWaterMark = gKernelQueueDepth;
  }
}

void updateSharedRuntimeSnapshot(uint32_t nowMs, bool incrementSeq) {
  portENTER_CRITICAL(&gSnapshotMux);
  if (incrementSeq) gSharedSnapshot.seq += 1;
  gSharedSnapshot.tsMs = nowMs;
  gSharedSnapshot.lastCompleteScanUs = gLastCompleteScanUs;
  gSharedSnapshot.maxCompleteScanUs = gMaxCompleteScanUs;
  gSharedSnapshot.scanBudgetUs = gScanBudgetUs;
  gSharedSnapshot.scanOverrunCount = gScanOverrunCount;
  gSharedSnapshot.scanOverrunLast = gScanOverrunLast;
  gSharedSnapshot.kernelQueueDepth = gKernelQueueDepth;
  gSharedSnapshot.kernelQueueHighWaterMark = gKernelQueueHighWaterMark;
  gSharedSnapshot.kernelQueueCapacity = gKernelQueueCapacity;
  gSharedSnapshot.commandLatencyLastUs = gCommandLatencyLastUs;
  gSharedSnapshot.commandLatencyMaxUs = gCommandLatencyMaxUs;
  gSharedSnapshot.rtcMinuteTickCount = gRtcMinuteTickCount;
  gSharedSnapshot.rtcIntentEnqueueCount = gRtcIntentEnqueueCount;
  gSharedSnapshot.rtcIntentEnqueueFailCount = gRtcIntentEnqueueFailCount;
  gSharedSnapshot.rtcLastEvalMs = gRtcLastEvalMs;
  gSharedSnapshot.mode = gRunMode;
  gSharedSnapshot.testModeActive = gTestModeActive;
  gSharedSnapshot.globalOutputMask = gGlobalOutputMask;
  gSharedSnapshot.breakpointPaused = gBreakpointPaused;
  gSharedSnapshot.scanCursor = gScanCursor;
  memcpy(gSharedSnapshot.cards, logicCards, sizeof(logicCards));
  memcpy(gSharedSnapshot.inputSource, gCardInputSource,
         sizeof(gCardInputSource));
  memcpy(gSharedSnapshot.forcedAIValue, gCardForcedAIValue,
         sizeof(gCardForcedAIValue));
  memcpy(gSharedSnapshot.outputMaskLocal, gCardOutputMask,
         sizeof(gCardOutputMask));
  memcpy(gSharedSnapshot.breakpointEnabled, gCardBreakpoint,
         sizeof(gCardBreakpoint));
  memcpy(gSharedSnapshot.setResult, gCardSetResult, sizeof(gCardSetResult));
  memcpy(gSharedSnapshot.resetResult, gCardResetResult,
         sizeof(gCardResetResult));
  memcpy(gSharedSnapshot.resetOverride, gCardResetOverride,
         sizeof(gCardResetOverride));
  memcpy(gSharedSnapshot.evalCounter, gCardEvalCounter,
         sizeof(gCardEvalCounter));
  portEXIT_CRITICAL(&gSnapshotMux);
}

uint8_t scanOrderCardIdFromCursor(uint16_t cursor) {
  return static_cast<uint8_t>((TOTAL_CARDS == 0) ? 0 : (cursor % TOTAL_CARDS));
}

bool isDoRunningState(cardState state) {
  return state == State_DO_OnDelay || state == State_DO_Active;
}

bool evalOperator(const LogicCard& target, logicOperator op,
                  uint32_t threshold) {
  switch (op) {
    case Op_AlwaysTrue:
      return true;
    case Op_AlwaysFalse:
      return false;
    case Op_LogicalTrue:
      return target.logicalState;
    case Op_LogicalFalse:
      return !target.logicalState;
    case Op_PhysicalOn:
      return target.physicalState;
    case Op_PhysicalOff:
      return !target.physicalState;
    case Op_Triggered:
      return target.triggerFlag;
    case Op_TriggerCleared:
      return !target.triggerFlag;
    case Op_GT:
      return target.currentValue > threshold;
    case Op_LT:
      return target.currentValue < threshold;
    case Op_EQ:
      return target.currentValue == threshold;
    case Op_NEQ:
      return target.currentValue != threshold;
    case Op_GTE:
      return target.currentValue >= threshold;
    case Op_LTE:
      return target.currentValue <= threshold;
    case Op_Running:
      return isDoRunningState(target.state);
    case Op_Finished:
      return target.state == State_DO_Finished;
    case Op_Stopped:
      return target.state == State_DO_Idle || target.state == State_DO_Finished;
    default:
      return false;
  }
}

bool evalCondition(uint8_t aId, logicOperator aOp, uint32_t aTh, uint8_t bId,
                   logicOperator bOp, uint32_t bTh, combineMode combine) {
  const LogicCard* aCard = getCardById(aId);
  bool aResult = (aCard != nullptr) ? evalOperator(*aCard, aOp, aTh) : false;
  if (combine == Combine_None) return aResult;

  const LogicCard* bCard = getCardById(bId);
  bool bResult = (bCard != nullptr) ? evalOperator(*bCard, bOp, bTh) : false;

  if (combine == Combine_AND) return aResult && bResult;
  if (combine == Combine_OR) return aResult || bResult;
  return false;
}

void resetDIRuntime(LogicCard& card) {
  card.logicalState = false;
  card.triggerFlag = false;
  card.currentValue = 0;
  card.startOnMs = 0;
  card.startOffMs = 0;
  card.repeatCounter = 0;
}

void processDICard(LogicCard& card, uint32_t nowMs) {
  bool sample = false;
  inputSourceMode sourceMode = InputSource_Real;
  if (card.id < TOTAL_CARDS) {
    sourceMode = gCardInputSource[card.id];
  }

  if (sourceMode == InputSource_ForcedHigh) {
    sample = true;
  } else if (sourceMode == InputSource_ForcedLow) {
    sample = false;
  } else if (card.hwPin != 255) {
    sample = (digitalRead(card.hwPin) == HIGH);
  }
  if (card.invert) sample = !sample;
  card.physicalState = sample;

  const bool setCondition = evalCondition(
      card.setA_ID, card.setA_Operator, card.setA_Threshold, card.setB_ID,
      card.setB_Operator, card.setB_Threshold, card.setCombine);
  const bool resetCondition =
      evalCondition(card.resetA_ID, card.resetA_Operator, card.resetA_Threshold,
                    card.resetB_ID, card.resetB_Operator, card.resetB_Threshold,
                    card.resetCombine);
  if (card.id < TOTAL_CARDS) {
    gCardSetResult[card.id] = setCondition;
    gCardResetResult[card.id] = resetCondition;
    gCardResetOverride[card.id] = setCondition && resetCondition;
  }

  if (resetCondition) {
    resetDIRuntime(card);
    card.state = State_DI_Inhibited;
    return;
  }

  if (!setCondition) {
    card.triggerFlag = false;
    card.state = State_DI_Idle;
    return;
  }

  bool previousSample = sample;
  if (card.id < TOTAL_CARDS) {
    if (gPrevDIPrimed[card.id]) {
      previousSample = gPrevDISample[card.id];
    }
    gPrevDISample[card.id] = sample;
    gPrevDIPrimed[card.id] = true;
  }

  const bool risingEdge = (!previousSample && sample);
  const bool fallingEdge = (previousSample && !sample);
  bool edgeMatchesMode = false;
  switch (card.mode) {
    case Mode_DI_Rising:
      edgeMatchesMode = risingEdge;
      break;
    case Mode_DI_Falling:
      edgeMatchesMode = fallingEdge;
      break;
    case Mode_DI_Change:
      edgeMatchesMode = risingEdge || fallingEdge;
      break;
    default:
      edgeMatchesMode = false;
      break;
  }

  if (!edgeMatchesMode) {
    card.triggerFlag = false;
    card.state = State_DI_Idle;
    return;
  }

  const uint32_t elapsed = nowMs - card.startOnMs;
  if (card.setting1 > 0 && elapsed < card.setting1) {
    card.triggerFlag = false;
    card.state = State_DI_Filtering;
    return;
  }

  card.triggerFlag = true;
  card.currentValue += 1;
  card.logicalState = sample;
  card.startOnMs = nowMs;
  card.state = State_DI_Qualified;
}

uint32_t clampUInt32(uint32_t value, uint32_t lo, uint32_t hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

void processAICard(LogicCard& card) {
  if (card.id < TOTAL_CARDS) {
    gCardSetResult[card.id] = false;
    gCardResetResult[card.id] = false;
    gCardResetOverride[card.id] = false;
  }
  uint32_t raw = 0;
  inputSourceMode sourceMode = InputSource_Real;
  if (card.id < TOTAL_CARDS) {
    sourceMode = gCardInputSource[card.id];
  }

  if (sourceMode == InputSource_ForcedValue) {
    raw = gCardForcedAIValue[card.id];
  } else if (card.hwPin != 255) {
    raw = static_cast<uint32_t>(analogRead(card.hwPin));
  }

  const uint32_t inMin =
      (card.setting1 < card.setting2) ? card.setting1 : card.setting2;
  const uint32_t inMax =
      (card.setting1 < card.setting2) ? card.setting2 : card.setting1;
  const uint32_t clamped = clampUInt32(raw, inMin, inMax);

  uint32_t scaled = card.startOnMs;
  if (inMax != inMin) {
    const int64_t outMin = static_cast<int64_t>(card.startOnMs);
    const int64_t outMax = static_cast<int64_t>(card.startOffMs);
    const int64_t outDelta = outMax - outMin;
    const int64_t inDelta = static_cast<int64_t>(inMax - inMin);
    const int64_t inOffset = static_cast<int64_t>(clamped - inMin);
    int64_t mapped = outMin + ((inOffset * outDelta) / inDelta);
    if (mapped < 0) mapped = 0;
    scaled = static_cast<uint32_t>(mapped);
  }

  const uint32_t alpha = (card.setting3 > 1000) ? 1000 : card.setting3;
  const uint64_t filtered =
      ((static_cast<uint64_t>(alpha) * scaled) +
       (static_cast<uint64_t>(1000 - alpha) * card.currentValue)) /
      1000ULL;
  card.currentValue = static_cast<uint32_t>(filtered);
  card.mode = Mode_AI_Continuous;
  card.state = State_AI_Streaming;
}

void forceDOIdle(LogicCard& card, bool clearCounter = false) {
  card.logicalState = false;
  card.physicalState = false;
  card.triggerFlag = false;
  card.startOnMs = 0;
  card.startOffMs = 0;
  card.repeatCounter = 0;
  if (clearCounter) card.currentValue = 0;
  card.state = State_DO_Idle;
}

void driveDOHardware(const LogicCard& card, bool driveHardware, bool level,
                     bool masked) {
  if (!driveHardware) return;
  if (card.hwPin == 255) return;
  if (masked) return;
  digitalWrite(card.hwPin, level ? HIGH : LOW);
}

void processDOCard(LogicCard& card, uint32_t nowMs, bool driveHardware) {
  const bool previousPhysical = card.physicalState;
  const bool setCondition = evalCondition(
      card.setA_ID, card.setA_Operator, card.setA_Threshold, card.setB_ID,
      card.setB_Operator, card.setB_Threshold, card.setCombine);
  const bool resetCondition =
      evalCondition(card.resetA_ID, card.resetA_Operator, card.resetA_Threshold,
                    card.resetB_ID, card.resetB_Operator, card.resetB_Threshold,
                    card.resetCombine);
  if (card.id < TOTAL_CARDS) {
    gCardSetResult[card.id] = setCondition;
    gCardResetResult[card.id] = resetCondition;
    gCardResetOverride[card.id] = setCondition && resetCondition;
  }

  bool prevSet = false;
  if (card.id < TOTAL_CARDS) {
    prevSet = gPrevSetCondition[card.id];
    gPrevSetCondition[card.id] = setCondition;
  }
  const bool setRisingEdge = setCondition && !prevSet;

  if (resetCondition) {
    forceDOIdle(card, true);
    driveDOHardware(card, driveHardware, false, isOutputMasked(card.id));
    return;
  }

  const bool retriggerable =
      (card.state == State_DO_Idle || card.state == State_DO_Finished);
  // Re-arm behavior: when DO/SIO is idle/finished, allow retrigger even if
  // setCondition stays high (level retrigger), while still supporting edge
  // trigger semantics for normal transitions.
  card.triggerFlag = retriggerable && (setRisingEdge || setCondition);

  if (card.triggerFlag) {
    card.logicalState = true;
    card.repeatCounter = 0;
    if (card.mode == Mode_DO_Immediate) {
      card.state = State_DO_Active;
      card.startOffMs = nowMs;
    } else {
      card.state = State_DO_OnDelay;
      card.startOnMs = nowMs;
    }
  }

  if (card.mode == Mode_DO_Gated && isDoRunningState(card.state) &&
      !setCondition) {
    forceDOIdle(card);
    driveDOHardware(card, driveHardware, false, isOutputMasked(card.id));
    return;
  }

  bool effectiveOutput = false;
  switch (card.state) {
    case State_DO_OnDelay: {
      effectiveOutput = false;
      if (card.setting1 == 0) {
        break;
      }
      if ((nowMs - card.startOnMs) >= card.setting1) {
        card.state = State_DO_Active;
        card.startOffMs = nowMs;
        effectiveOutput = true;
      }
      break;
    }
    case State_DO_Active: {
      effectiveOutput = true;
      if (card.setting2 == 0) {
        break;
      }
      if ((nowMs - card.startOffMs) >= card.setting2) {
        card.repeatCounter += 1;
        effectiveOutput = false;

        if (card.setting3 == 0) {
          card.state = State_DO_OnDelay;
          card.startOnMs = nowMs;
          break;
        }

        if (card.repeatCounter >= card.setting3) {
          card.logicalState = false;
          card.state = State_DO_Finished;
          break;
        }

        card.state = State_DO_OnDelay;
        card.startOnMs = nowMs;
      }
      break;
    }
    case State_DO_Finished:
    case State_DO_Idle:
    default:
      effectiveOutput = false;
      break;
  }

  // DO/SIO cycle counter: count each OFF->ON transition of effective output.
  if (!previousPhysical && effectiveOutput) {
    card.currentValue += 1;
  }

  card.physicalState = effectiveOutput;
  driveDOHardware(card, driveHardware, effectiveOutput,
                  isOutputMasked(card.id));
}

void processSIOCard(LogicCard& card, uint32_t nowMs) {
  processDOCard(card, nowMs, false);
}

void processMathCard(LogicCard& card) {
  const bool setCondition = evalCondition(
      card.setA_ID, card.setA_Operator, card.setA_Threshold, card.setB_ID,
      card.setB_Operator, card.setB_Threshold, card.setCombine);
  const bool resetCondition =
      evalCondition(card.resetA_ID, card.resetA_Operator, card.resetA_Threshold,
                    card.resetB_ID, card.resetB_Operator, card.resetB_Threshold,
                    card.resetCombine);
  if (card.id < TOTAL_CARDS) {
    gCardSetResult[card.id] = setCondition;
    gCardResetResult[card.id] = resetCondition;
    gCardResetOverride[card.id] = setCondition && resetCondition;
  }

  if (resetCondition) {
    card.logicalState = false;
    card.physicalState = false;
    card.triggerFlag = false;
    card.currentValue = card.setting3;
    card.state = State_None;
    return;
  }

  if (!setCondition) {
    card.logicalState = false;
    card.physicalState = false;
    card.triggerFlag = false;
    card.state = State_None;
    return;
  }

  card.logicalState = true;
  card.physicalState = true;
  card.triggerFlag = true;

  // Transitional deterministic math bridge: constant inputA/inputB add path.
  const uint64_t raw =
      static_cast<uint64_t>(card.setting1) + static_cast<uint64_t>(card.setting2);
  uint32_t value =
      (raw > static_cast<uint64_t>(UINT32_MAX)) ? UINT32_MAX : static_cast<uint32_t>(raw);
  if (card.startOffMs >= card.startOnMs) {
    value = clampUInt32(value, card.startOnMs, card.startOffMs);
  }
  card.currentValue = value;
  card.state = State_None;
}

void processRtcCard(LogicCard& card) {
  if (card.id < TOTAL_CARDS) {
    gCardSetResult[card.id] = false;
    gCardResetResult[card.id] = false;
    gCardResetOverride[card.id] = false;
  }
  card.mode = Mode_None;
  card.state = State_None;
  card.physicalState = card.logicalState;
  if (!card.logicalState) card.triggerFlag = false;
}

void processCardById(uint8_t cardId, uint32_t nowMs) {
  if (cardId >= TOTAL_CARDS) return;
  LogicCard& card = logicCards[cardId];
  switch (card.type) {
    case DigitalInput:
      processDICard(card, nowMs);
      return;
    case AnalogInput:
      processAICard(card);
      return;
    case SoftIO:
      processSIOCard(card, nowMs);
      return;
    case DigitalOutput:
      processDOCard(card, nowMs, true);
      return;
    case MathCard:
      processMathCard(card);
      return;
    case RtcCard:
      processRtcCard(card);
      return;
    default:
      return;
  }
}

void processOneScanOrderedCard(uint32_t nowMs, bool honorBreakpoints) {
  uint8_t cardId = scanOrderCardIdFromCursor(gScanCursor);
  processCardById(cardId, nowMs);
  gCardEvalCounter[cardId] += 1;

  gScanCursor = static_cast<uint16_t>((gScanCursor + 1) % TOTAL_CARDS);

  if (honorBreakpoints && gRunMode == RUN_BREAKPOINT &&
      gCardBreakpoint[cardId]) {
    gBreakpointPaused = true;
  }
}

bool runFullScanCycle(uint32_t nowMs, bool honorBreakpoints) {
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    processOneScanOrderedCard(nowMs, honorBreakpoints);
    if (gBreakpointPaused) return false;
  }
  return true;
}

void runEngineIteration(uint32_t nowMs, uint32_t& lastScanMs) {
  processKernelCommandQueue();
  if (gKernelPauseRequested) {
    gKernelPaused = true;
    updateSharedRuntimeSnapshot(nowMs, false);
    return;
  }
  gKernelPaused = false;
  if (lastScanMs == 0) {
    lastScanMs = nowMs;
  }

  uint32_t scanInterval = gScanIntervalMs;
  gScanBudgetUs = scanInterval * 1000;
  if ((nowMs - lastScanMs) < scanInterval) {
    updateSharedRuntimeSnapshot(nowMs, false);
    return;
  }
  lastScanMs += scanInterval;

  if (gRunMode == RUN_STEP) {
    if (gStepRequested) {
      processOneScanOrderedCard(nowMs, false);
      gStepRequested = false;
      updateSharedRuntimeSnapshot(nowMs, true);
      return;
    }
    updateSharedRuntimeSnapshot(nowMs, false);
    return;
  }

  if (gRunMode == RUN_BREAKPOINT && gBreakpointPaused) {
    updateSharedRuntimeSnapshot(nowMs, false);
    return;
  }

  uint32_t scanStartUs = micros();
  bool completedFullScan = runFullScanCycle(nowMs, gRunMode == RUN_BREAKPOINT);
  uint32_t scanEndUs = micros();
  if (completedFullScan) {
    gLastCompleteScanUs = (scanEndUs - scanStartUs);
    if (gLastCompleteScanUs > gMaxCompleteScanUs) {
      gMaxCompleteScanUs = gLastCompleteScanUs;
    }
    gScanOverrunLast = (gLastCompleteScanUs > gScanBudgetUs);
    if (gScanOverrunLast) gScanOverrunCount += 1;
  }
  updateSharedRuntimeSnapshot(nowMs, true);
}

bool rtcFieldMatches(int fieldValue, int scheduleValue) {
  return scheduleValue < 0 || fieldValue == scheduleValue;
}

bool rtcChannelMatchesMinute(const RtcScheduleChannel& channel,
                             const DateTime& dt) {
  if (!channel.enabled) return false;
  return rtcFieldMatches(dt.year(), channel.year) &&
         rtcFieldMatches(dt.month(), channel.month) &&
         rtcFieldMatches(dt.day(), channel.day) &&
         rtcFieldMatches(dt.dayOfTheWeek(), channel.weekday) &&
         rtcFieldMatches(dt.hour(), channel.hour) &&
         rtcFieldMatches(dt.minute(), channel.minute);
}

void serviceRtcMinuteScheduler(uint32_t nowMs) {
  if (!gRtcClockInitialized) return;

  static uint32_t lastPollMs = 0;
  if ((nowMs - lastPollMs) < 200) return;
  lastPollMs = nowMs;

  DateTime now = gRtcClock.now();
  const int32_t minuteKey =
      (((now.year() * 100 + now.month()) * 100 + now.day()) * 100 +
       now.hour()) *
          100 +
      now.minute();
  if (minuteKey == gRtcLastMinuteKey) return;

  gRtcLastMinuteKey = minuteKey;
  gRtcMinuteTickCount += 1;
  gRtcLastEvalMs = nowMs;

  // Default RTC schedule outputs to false each minute; matching channels
  // re-assert true so downstream conditions can reference RTC cards directly.
  for (uint8_t i = 0; i < NUM_RTC_SCHED_CHANNELS; ++i) {
    const RtcScheduleChannel& channel = gRtcScheduleChannels[i];
    if (!channel.enabled) continue;
    if (channel.rtcCardId >= TOTAL_CARDS) {
      gRtcIntentEnqueueFailCount += 1;
      continue;
    }
    KernelCommand clearCmd = {};
    clearCmd.type = KernelCmd_SetRtcCardState;
    clearCmd.cardId = channel.rtcCardId;
    clearCmd.flag = false;
    if (enqueueKernelCommand(clearCmd)) {
      gRtcIntentEnqueueCount += 1;
    } else {
      gRtcIntentEnqueueFailCount += 1;
    }
  }

  for (uint8_t i = 0; i < NUM_RTC_SCHED_CHANNELS; ++i) {
    const RtcScheduleChannel& channel = gRtcScheduleChannels[i];
    if (!rtcChannelMatchesMinute(channel, now)) continue;
    KernelCommand cmd = {};
    cmd.type = KernelCmd_SetRtcCardState;
    cmd.cardId = channel.rtcCardId;
    cmd.flag = true;
    if (enqueueKernelCommand(cmd)) {
      gRtcIntentEnqueueCount += 1;
    } else {
      gRtcIntentEnqueueFailCount += 1;
    }
  }
}

void core0EngineTask(void* param) {
  (void)param;
  uint32_t lastScanMs = 0;
  for (;;) {
    runEngineIteration(millis(), lastScanMs);
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void core1PortalTask(void* param) {
  (void)param;
  bool wifiOk = connectWiFiWithPolicy();
  if (wifiOk) {
    initPortalServer();
    initWebSocketServer();
  }
  for (;;) {
    if (wifiOk) {
      if (gPortalReconnectRequested) {
        gPortalReconnectRequested = false;
        wifiOk = connectWiFiWithPolicy();
        if (wifiOk) {
          initPortalServer();
          initWebSocketServer();
        }
      }
      handlePortalServerLoop();
      handleWebSocketLoop();
      publishRuntimeSnapshotWebSocket();
      serviceRtcMinuteScheduler(millis());
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }
    // Optional low-frequency retry in offline mode.
    static uint32_t lastRetryMs = 0;
    uint32_t nowMs = millis();
    if (nowMs - lastRetryMs >= 30000) {
      lastRetryMs = nowMs;
      wifiOk = connectWiFiWithPolicy();
      if (wifiOk) {
        initPortalServer();
        initWebSocketServer();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void setup() {
  Serial.begin(115200);
  gRtcClock.begin(DateTime(F(__DATE__), F(__TIME__)));
  gRtcClockInitialized = true;
  configureHardwarePinsSafeState();
  initializeRuntimeControlState();

  bool fsReady = LittleFS.begin(true);
  if (!fsReady) {
    Serial.println("LittleFS mount failed");
    initializeAllCardsSafeDefaults();
  } else {
    if (!loadPortalSettingsFromLittleFS()) {
      savePortalSettingsToLittleFS();
    }
    bootstrapCardsFromStorage();
  }

  gKernelCommandQueue = xQueueCreate(16, sizeof(KernelCommand));
  if (gKernelCommandQueue == nullptr) {
    Serial.println("Failed to create kernel command queue");
    return;
  }
  gKernelQueueCapacity =
      static_cast<uint16_t>(uxQueueMessagesWaiting(gKernelCommandQueue) +
                            uxQueueSpacesAvailable(gKernelCommandQueue));

  updateSharedRuntimeSnapshot(millis(), false);

  xTaskCreatePinnedToCore(core0EngineTask, "core0_engine", 8192, nullptr, 3,
                          &gCore0TaskHandle, 0);
  xTaskCreatePinnedToCore(core1PortalTask, "core1_portal", 8192, nullptr, 1,
                          &gCore1TaskHandle, 1);
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }

bool applyCommand(JsonObjectConst command) {
  const char* name = command["name"] | "";
  JsonObjectConst payload = command["payload"].as<JsonObjectConst>();
  KernelCommand kernelCommand = {};

  if (strcmp(name, "set_run_mode") == 0) {
    const char* mode = payload["mode"] | "RUN_NORMAL";
    kernelCommand.type = KernelCmd_SetRunMode;
    bool modeMatched = false;
    if (strcmp(mode, "RUN_NORMAL") == 0) {
      kernelCommand.mode = RUN_NORMAL;
      modeMatched = true;
    }
    if (strcmp(mode, "RUN_STEP") == 0) {
      kernelCommand.mode = RUN_STEP;
      modeMatched = true;
    }
    if (strcmp(mode, "RUN_BREAKPOINT") == 0) {
      kernelCommand.mode = RUN_BREAKPOINT;
      modeMatched = true;
    }
    if (!modeMatched) return false;
    return enqueueKernelCommand(kernelCommand);
  }

  if (strcmp(name, "step_once") == 0) {
    kernelCommand.type = KernelCmd_StepOnce;
    return enqueueKernelCommand(kernelCommand);
  }

  if (strcmp(name, "set_breakpoint") == 0) {
    kernelCommand.type = KernelCmd_SetBreakpoint;
    kernelCommand.cardId = payload["cardId"] | 255;
    kernelCommand.flag = payload["enabled"] | false;
    return enqueueKernelCommand(kernelCommand);
  }

  if (strcmp(name, "set_test_mode") == 0) {
    kernelCommand.type = KernelCmd_SetTestMode;
    kernelCommand.flag = payload["active"] | false;
    return enqueueKernelCommand(kernelCommand);
  }

  if (strcmp(name, "set_input_force") == 0) {
    uint8_t cardId = payload["cardId"] | 255;
    bool forced = payload["forced"] | false;
    kernelCommand.type = KernelCmd_SetInputForce;
    kernelCommand.cardId = cardId;
    if (!forced) {
      kernelCommand.inputMode = InputSource_Real;
      kernelCommand.value = 0;
      return enqueueKernelCommand(kernelCommand);
    }

    if (isDigitalInputCard(cardId)) {
      bool value = payload["value"] | false;
      kernelCommand.inputMode =
          value ? InputSource_ForcedHigh : InputSource_ForcedLow;
      kernelCommand.value = 0;
      return enqueueKernelCommand(kernelCommand);
    }
    if (isAnalogInputCard(cardId)) {
      kernelCommand.inputMode = InputSource_ForcedValue;
      kernelCommand.value = payload["value"] | 0;
      return enqueueKernelCommand(kernelCommand);
    }
    return false;
  }

  if (strcmp(name, "set_output_mask") == 0) {
    kernelCommand.type = KernelCmd_SetOutputMask;
    kernelCommand.cardId = payload["cardId"] | 255;
    kernelCommand.flag = payload["masked"] | false;
    return enqueueKernelCommand(kernelCommand);
  }

  if (strcmp(name, "set_output_mask_global") == 0) {
    kernelCommand.type = KernelCmd_SetOutputMaskGlobal;
    kernelCommand.flag = payload["masked"] | false;
    return enqueueKernelCommand(kernelCommand);
  }

  return false;
}
