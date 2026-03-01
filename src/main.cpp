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
#include "kernel/legacy_card_profile.h"
#include "kernel/legacy_config_validator.h"
#include "kernel/v3_card_bridge.h"
#include "kernel/v3_ai_runtime.h"
#include "kernel/v3_card_types.h"
#include "kernel/v3_di_runtime.h"
#include "kernel/v3_do_runtime.h"
#include "kernel/v3_math_runtime.h"
#include "kernel/v3_sio_runtime.h"
#include "kernel/v3_rtc_runtime.h"
#include "kernel/v3_runtime_store.h"
#include "kernel/v3_runtime_signals.h"
#include "kernel/v3_status_runtime.h"
#include "portal/routes.h"
#include "runtime/shared_snapshot.h"
#include "runtime/runtime_card_meta.h"
#include "runtime/snapshot_card_builder.h"
#include "runtime/snapshot_json.h"
#include "storage/v3_config_service.h"
#include "storage/config_lifecycle.h"
#include "storage/v3_normalizer.h"

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

const LegacyCardProfileLayout kLegacyCardLayout = {
    TOTAL_CARDS, DO_START, AI_START, SIO_START, MATH_START, RTC_START,
    DI_Pins,      DO_Pins,  AI_Pins,  SIO_Pins};

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
V3DiRuntimeState gDiRuntime[NUM_DI] = {};
V3DoRuntimeState gDoRuntime[NUM_DO] = {};
V3AiRuntimeState gAiRuntime[NUM_AI] = {};
V3SioRuntimeState gSioRuntime[NUM_SIO] = {};
V3MathRuntimeState gMathRuntime[NUM_MATH] = {};
V3RtcRuntimeState gRtcRuntime[NUM_RTC] = {};
V3RuntimeStoreView gRuntimeStore = {gDiRuntime, NUM_DI,   gDoRuntime, NUM_DO,
                                    gAiRuntime, NUM_AI,   gSioRuntime, NUM_SIO,
                                    gMathRuntime, NUM_MATH, gRtcRuntime, NUM_RTC};
V3CardConfig gActiveTypedCards[TOTAL_CARDS] = {};
V3RuntimeSignal gRuntimeSignals[TOTAL_CARDS] = {};
RuntimeCardMeta gRuntimeCardMeta[TOTAL_CARDS] = {};
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

using RtcScheduleChannel = V3RtcScheduleChannel;

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
void initializeCardArraySafeDefaults(LogicCard* cards);
void refreshActiveTypedCardsFromLegacy();
const V3CardConfig* activeTypedCardConfig(uint8_t cardId);
const RtcScheduleChannel* findRtcScheduleByCardId(uint8_t cardId);
bool deserializeCardsFromArray(JsonArrayConst array, LogicCard* outCards);
bool validateConfigCardsArray(JsonArrayConst array, String& reason);
void serviceRtcMinuteScheduler(uint32_t nowMs);
void syncRuntimeStateFromCards();
void writeConfigResultResponse(int statusCode, bool ok, const char* requestId,
                               const char* errorCode, const String& message,
                               JsonObject* extra = nullptr);
void serializeCardsToV3Array(const LogicCard* sourceCards, JsonArray& cards);
void buildV3ConfigEnvelope(const LogicCard* sourceCards, JsonDocument& doc,
                           const char* configId, const char* requestId);

void initializeAllCardsSafeDefaults() {
  profileInitializeCardArraySafeDefaults(logicCards, kLegacyCardLayout);
  syncRuntimeStateFromCards();
  refreshRuntimeSignalsFromRuntime(gRuntimeCardMeta, gRuntimeStore,
                                   gRuntimeSignals, TOTAL_CARDS);
}

void refreshActiveTypedCardsFromLegacy() {
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    const LogicCard& card = logicCards[i];
    const RtcScheduleChannel* rtc = findRtcScheduleByCardId(card.id);
    const int16_t rtcYear = (rtc != nullptr) ? rtc->year : -1;
    const int8_t rtcMonth = (rtc != nullptr) ? rtc->month : -1;
    const int8_t rtcDay = (rtc != nullptr) ? rtc->day : -1;
    const int8_t rtcWeekday = (rtc != nullptr) ? rtc->weekday : -1;
    const int8_t rtcHour = (rtc != nullptr) ? rtc->hour : -1;
    const int8_t rtcMinute = (rtc != nullptr) ? rtc->minute : -1;
    legacyToV3CardConfig(card, rtcYear, rtcMonth, rtcDay, rtcWeekday, rtcHour,
                         rtcMinute, gActiveTypedCards[i]);
  }
}

void syncRuntimeStateFromCards() {
  refreshActiveTypedCardsFromLegacy();
  syncRuntimeStoreFromTypedCards(logicCards, gActiveTypedCards, TOTAL_CARDS,
                                 gRuntimeStore);
  refreshRuntimeCardMetaFromTypedCards(gActiveTypedCards, TOTAL_CARDS, DO_START,
                                       AI_START, SIO_START, MATH_START,
                                       RTC_START, gRuntimeCardMeta);
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    mirrorRuntimeStoreCardToLegacyByTyped(logicCards[i], gActiveTypedCards[i],
                                          gRuntimeStore);
  }
}

bool saveLogicCardsToLittleFS() {
  return saveCardsToPath(kConfigPath, logicCards);
}

bool loadLogicCardsFromLittleFS() {
  LogicCard loaded[TOTAL_CARDS];
  if (!loadCardsFromPath(kConfigPath, loaded)) return false;
  memcpy(logicCards, loaded, sizeof(logicCards));
  syncRuntimeStateFromCards();
  refreshRuntimeSignalsFromRuntime(gRuntimeCardMeta, gRuntimeStore,
                                   gRuntimeSignals, TOTAL_CARDS);
  return true;
}

void printLogicCardsJsonToSerial(const char* label) {
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  for (uint8_t i = 0; i < TOTAL_CARDS; ++i) {
    JsonObject obj = array.add<JsonObject>();
    profileSerializeCardToJson(logicCards[i], obj);
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
  const RuntimeSnapshotCard& card = snapshot.cards[cardId];
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
  V3ConfigContext configContext = {};
  LogicCard baseline[TOTAL_CARDS];
  initializeCardArraySafeDefaults(baseline);
  String reason;
  const char* errorCode = "VALIDATION_FAILED";
  const char* requestId = root["requestId"] | "";
  const V3CardLayout layout = {TOTAL_CARDS, DO_START, AI_START,
                               SIO_START,  MATH_START, RTC_START};
  if (!normalizeV3ConfigRequestContext(
          root, layout, kApiVersion, kSchemaVersion, baseline, TOTAL_CARDS,
          NUM_RTC_SCHED_CHANNELS, configContext, reason, errorCode)) {
    writeConfigResultResponse(400, false, requestId, errorCode, reason);
    return;
  }
  applyRtcScheduleChannelsFromConfig(configContext.rtcChannels,
                                     configContext.rtcCount,
                                     gRtcScheduleChannels,
                                     NUM_RTC_SCHED_CHANNELS);

  LogicCard stagedCards[TOTAL_CARDS];
  if (!buildLegacyCardsFromTypedWithBaseline(
          configContext.typedCards, configContext.typedCount, baseline,
          TOTAL_CARDS, stagedCards,
          reason)) {
    writeConfigResultResponse(500, false, requestId, "INTERNAL_ERROR",
                              reason);
    return;
  }

  JsonDocument stagedDoc;
  buildV3ConfigEnvelope(stagedCards, stagedDoc, "staged", requestId);

  if (!writeJsonToPath(kStagedConfigPath, stagedDoc)) {
    writeConfigResultResponse(500, false, requestId, "COMMIT_FAILED",
                              "failed to save staged file");
    return;
  }

  JsonDocument extras;
  extras["stagedVersion"] = "staged";
  JsonObject extrasObj = extras.as<JsonObject>();
  writeConfigResultResponse(200, true, requestId, nullptr, "", &extrasObj);
}

void handleHttpStagedValidateConfig() {
  JsonDocument source;
  V3ConfigContext configContext = {};
  LogicCard baseline[TOTAL_CARDS];
  initializeCardArraySafeDefaults(baseline);
  String reason;
  const char* requestId = "";

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
    const V3CardLayout layout = {TOTAL_CARDS, DO_START, AI_START,
                                 SIO_START,  MATH_START, RTC_START};
    if (!normalizeV3ConfigRequestContext(
            root, layout, kApiVersion, kSchemaVersion, baseline, TOTAL_CARDS,
            NUM_RTC_SCHED_CHANNELS, configContext, reason, errorCode)) {
      writeConfigResultResponse(400, false, requestId, errorCode, reason);
      return;
    }
    applyRtcScheduleChannelsFromConfig(configContext.rtcChannels,
                                       configContext.rtcCount,
                                       gRtcScheduleChannels,
                                       NUM_RTC_SCHED_CHANNELS);
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
    const V3CardLayout layout = {TOTAL_CARDS, DO_START, AI_START,
                                 SIO_START,  MATH_START, RTC_START};
    if (!normalizeV3ConfigRequestContext(
            root, layout, kApiVersion, kSchemaVersion, baseline, TOTAL_CARDS,
            NUM_RTC_SCHED_CHANNELS, configContext, reason, errorCode)) {
      writeConfigResultResponse(400, false, requestId, errorCode, reason);
      return;
    }
    applyRtcScheduleChannelsFromConfig(configContext.rtcChannels,
                                       configContext.rtcCount,
                                       gRtcScheduleChannels,
                                       NUM_RTC_SCHED_CHANNELS);
  }

  JsonDocument extras;
  JsonObject validation = extras["validation"].to<JsonObject>();
  validation["errors"].to<JsonArray>();
  validation["warnings"].to<JsonArray>();
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

bool commitCards(const V3ConfigContext& configContext, String& reason) {
  LogicCard baseline[TOTAL_CARDS];
  initializeCardArraySafeDefaults(baseline);
  LogicCard nextCards[TOTAL_CARDS];
  if (!buildLegacyCardsFromTypedWithBaseline(
          configContext.typedCards, configContext.typedCount, baseline,
          TOTAL_CARDS, nextCards, reason)) {
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

bool commitLegacyCards(JsonArrayConst cards, String& reason) {
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
  V3ConfigContext configContext = {};
  LogicCard baseline[TOTAL_CARDS];
  initializeCardArraySafeDefaults(baseline);
  String reason;
  const char* requestId = "";
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
    const V3CardLayout layout = {TOTAL_CARDS, DO_START, AI_START,
                                 SIO_START,  MATH_START, RTC_START};
    if (!normalizeV3ConfigRequestContext(
            root, layout, kApiVersion, kSchemaVersion, baseline, TOTAL_CARDS,
            NUM_RTC_SCHED_CHANNELS, configContext, reason, errorCode)) {
      writeConfigResultResponse(400, false, requestId, errorCode, reason);
      return;
    }
    applyRtcScheduleChannelsFromConfig(configContext.rtcChannels,
                                       configContext.rtcCount,
                                       gRtcScheduleChannels,
                                       NUM_RTC_SCHED_CHANNELS);
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
    const V3CardLayout layout = {TOTAL_CARDS, DO_START, AI_START,
                                 SIO_START,  MATH_START, RTC_START};
    if (!normalizeV3ConfigRequestContext(
            root, layout, kApiVersion, kSchemaVersion, baseline, TOTAL_CARDS,
            NUM_RTC_SCHED_CHANNELS, configContext, reason, errorCode)) {
      writeConfigResultResponse(400, false, requestId, errorCode, reason);
      return;
    }
    applyRtcScheduleChannelsFromConfig(configContext.rtcChannels,
                                       configContext.rtcCount,
                                       gRtcScheduleChannels,
                                       NUM_RTC_SCHED_CHANNELS);
  }

  if (!commitCards(configContext, reason)) {
    writeConfigResultResponse(500, false, requestId, "COMMIT_FAILED", reason);
    return;
  }

  JsonDocument extras;
  extras["activeVersion"] = gActiveVersion;
  JsonObject head = extras["historyHead"].to<JsonObject>();
  writeHistoryHead(head);
  extras["requiresRestart"] = false;
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
  if (!commitLegacyCards(cards, reason)) {
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
  return id < TOTAL_CARDS && gActiveTypedCards[id].family == V3CardFamily::DI;
}

bool isDigitalOutputCard(uint8_t id) {
  return id < TOTAL_CARDS && gActiveTypedCards[id].family == V3CardFamily::DO;
}

bool isAnalogInputCard(uint8_t id) {
  return id < TOTAL_CARDS && gActiveTypedCards[id].family == V3CardFamily::AI;
}

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
  profileInitializeCardArraySafeDefaults(cards, kLegacyCardLayout);
}

bool deserializeCardsFromArray(JsonArrayConst array, LogicCard* outCards) {
  return profileDeserializeCardsFromArray(array, outCards, kLegacyCardLayout);
}

bool validateConfigCardsArray(JsonArrayConst array, String& reason) {
  return validateLegacyConfigCardsArray(array, TOTAL_CARDS, DO_START, AI_START,
                                        SIO_START, MATH_START, RTC_START,
                                        reason);
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
  V3ConfigContext configContext = {};
  LogicCard baseline[TOTAL_CARDS];
  initializeCardArraySafeDefaults(baseline);
  String reason;
  const char* errorCode = "VALIDATION_FAILED";
  const V3CardLayout layout = {TOTAL_CARDS, DO_START, AI_START,
                               SIO_START,  MATH_START, RTC_START};
  if (!normalizeV3ConfigRequestContext(
          doc.as<JsonObjectConst>(), layout, kApiVersion, kSchemaVersion,
          baseline, TOTAL_CARDS, NUM_RTC_SCHED_CHANNELS, configContext, reason,
          errorCode)) {
    return false;
  }
  applyRtcScheduleChannelsFromConfig(configContext.rtcChannels,
                                     configContext.rtcCount,
                                     gRtcScheduleChannels,
                                     NUM_RTC_SCHED_CHANNELS);
  return buildLegacyCardsFromTypedWithBaseline(
      configContext.typedCards, configContext.typedCount, baseline, TOTAL_CARDS,
      outCards, reason);
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
  syncRuntimeStateFromCards();
  refreshRuntimeSignalsFromRuntime(gRuntimeCardMeta, gRuntimeStore,
                                   gRuntimeSignals, TOTAL_CARDS);
  memset(gPrevDISample, 0, sizeof(gPrevDISample));
  memset(gPrevDIPrimed, 0, sizeof(gPrevDIPrimed));
  updateSharedRuntimeSnapshot(millis(), false);
  resumeKernelAfterConfigApply();
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
  const V3CardConfig* cfgCard = activeTypedCardConfig(cardId);
  if (cfgCard == nullptr || cfgCard->family != V3CardFamily::RTC) return false;
  LogicCard& card = logicCards[cardId];
  if (cardId < RTC_START) return false;
  const uint8_t rtcIndex = static_cast<uint8_t>(cardId - RTC_START);
  V3RtcRuntimeState* runtime = runtimeRtcStateAt(rtcIndex, gRuntimeStore);
  if (runtime == nullptr) return false;

  runtime->logicalState = state;
  runtime->physicalState = state;
  runtime->triggerFlag = state;  // one-minute assertion edge for condition logic
  runtime->currentValue = state ? 1U : 0U;
  if (state) {
    runtime->triggerStartMs = millis();
  }
  if (!state) {
    runtime->triggerFlag = false;
    runtime->triggerStartMs = 0;
  }
  mirrorRuntimeStoreCardToLegacyByTyped(card, *cfgCard, gRuntimeStore);
  refreshRuntimeSignalAt(gRuntimeCardMeta, gRuntimeStore, gRuntimeSignals,
                         TOTAL_CARDS, cardId);
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
  buildRuntimeSnapshotCards(gRuntimeCardMeta, TOTAL_CARDS, gRuntimeStore,
                            gSharedSnapshot.cards);
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

bool evalOperator(const V3RuntimeSignal& target, logicOperator op,
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
      return isMissionRunning(target.type, target.state);
    case Op_Finished:
      return isMissionFinished(target.type, target.state);
    case Op_Stopped:
      return isMissionStopped(target.type, target.state);
    default:
      return false;
  }
}

bool evalCondition(uint8_t aId, logicOperator aOp, uint32_t aTh, uint8_t bId,
                   logicOperator bOp, uint32_t bTh, combineMode combine) {
  const V3RuntimeSignal* aSignal = (aId < TOTAL_CARDS) ? &gRuntimeSignals[aId] : nullptr;
  bool aResult =
      (aSignal != nullptr) ? evalOperator(*aSignal, aOp, aTh) : false;
  if (combine == Combine_None) return aResult;

  const V3RuntimeSignal* bSignal = (bId < TOTAL_CARDS) ? &gRuntimeSignals[bId] : nullptr;
  bool bResult =
      (bSignal != nullptr) ? evalOperator(*bSignal, bOp, bTh) : false;

  if (combine == Combine_AND) return aResult && bResult;
  if (combine == Combine_OR) return aResult || bResult;
  return false;
}

const V3CardConfig* activeTypedCardConfig(uint8_t cardId) {
  if (cardId >= TOTAL_CARDS) return nullptr;
  return &gActiveTypedCards[cardId];
}

bool evalTypedConditionBlock(const V3ConditionBlock& block) {
  return evalCondition(block.clauseAId, block.clauseAOperator,
                       block.clauseAThreshold, block.clauseBId,
                       block.clauseBOperator, block.clauseBThreshold,
                       block.combiner);
}

void processDICard(uint8_t cardId, uint32_t nowMs) {
  if (cardId >= TOTAL_CARDS) return;
  const V3CardConfig* cfgCard = activeTypedCardConfig(cardId);
  if (cfgCard == nullptr || cfgCard->family != V3CardFamily::DI) return;
  const V3DiConfig& cfgTyped = cfgCard->di;
  LogicCard& card = logicCards[cardId];
  V3DiRuntimeState* runtime = runtimeDiStateAt(cfgTyped.channel, gRuntimeStore);
  if (runtime == nullptr) return;

  bool sample = false;
  inputSourceMode sourceMode = InputSource_Real;
  sourceMode = gCardInputSource[cardId];
  uint8_t hwPin = 255;
  if (cfgTyped.channel < NUM_DI) hwPin = DI_Pins[cfgTyped.channel];

  if (sourceMode == InputSource_ForcedHigh) {
    sample = true;
  } else if (sourceMode == InputSource_ForcedLow) {
    sample = false;
  } else if (hwPin != 255) {
    sample = (digitalRead(hwPin) == HIGH);
  }
  if (cfgTyped.invert) sample = !sample;

  const bool setCondition = evalTypedConditionBlock(cfgTyped.set);
  const bool resetCondition = evalTypedConditionBlock(cfgTyped.reset);

  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = cfgTyped.debounceTimeMs;
  cfg.edgeMode = cfgTyped.edgeMode;

  V3DiStepInput in = {};
  in.nowMs = nowMs;
  in.sample = sample;
  in.setCondition = setCondition;
  in.resetCondition = resetCondition;
  in.prevSample = gPrevDISample[cardId];
  in.prevSampleValid = gPrevDIPrimed[cardId];

  V3DiStepOutput out = {};
  runV3DiStep(cfg, *runtime, in, out);

  mirrorRuntimeStoreCardToLegacyByTyped(card, *cfgCard, gRuntimeStore);

  gPrevDISample[cardId] = out.nextPrevSample;
  gPrevDIPrimed[cardId] = out.nextPrevSampleValid;
  gCardSetResult[cardId] = out.setResult;
  gCardResetResult[cardId] = out.resetResult;
  gCardResetOverride[cardId] = out.resetOverride;
}

void processAICard(uint8_t cardId) {
  if (cardId >= TOTAL_CARDS) return;
  const V3CardConfig* cfgCard = activeTypedCardConfig(cardId);
  if (cfgCard == nullptr || cfgCard->family != V3CardFamily::AI) return;
  const V3AiConfig& cfgTyped = cfgCard->ai;
  LogicCard& card = logicCards[cardId];
  V3AiRuntimeState* runtime = runtimeAiStateAt(cfgTyped.channel, gRuntimeStore);
  if (runtime == nullptr) return;
  gCardSetResult[cardId] = false;
  gCardResetResult[cardId] = false;
  gCardResetOverride[cardId] = false;
  uint32_t raw = 0;
  inputSourceMode sourceMode = InputSource_Real;
  sourceMode = gCardInputSource[cardId];

  if (sourceMode == InputSource_ForcedValue) {
    raw = gCardForcedAIValue[cardId];
  } else if (cfgTyped.channel < NUM_AI) {
    raw = static_cast<uint32_t>(analogRead(AI_Pins[cfgTyped.channel]));
  }

  V3AiRuntimeConfig cfg = {};
  cfg.inputMin = cfgTyped.inputMin;
  cfg.inputMax = cfgTyped.inputMax;
  cfg.outputMin = cfgTyped.outputMin;
  cfg.outputMax = cfgTyped.outputMax;
  cfg.emaAlphaX1000 = cfgTyped.emaAlphaX100 * 10U;

  V3AiStepInput in = {};
  in.rawSample = raw;

  runV3AiStep(cfg, *runtime, in);

  mirrorRuntimeStoreCardToLegacyByTyped(card, *cfgCard, gRuntimeStore);
}

void processDOCard(uint8_t cardId, uint32_t nowMs, bool driveHardware) {
  if (cardId >= TOTAL_CARDS) return;
  const V3CardConfig* cfgCard = activeTypedCardConfig(cardId);
  if (cfgCard == nullptr || cfgCard->family != V3CardFamily::DO) return;
  const V3DoConfig& cfgTyped = cfgCard->dout;
  LogicCard& card = logicCards[cardId];
  V3DoRuntimeState* runtime = runtimeDoStateAt(cfgTyped.channel, gRuntimeStore);
  if (runtime == nullptr) return;

  const bool setCondition = evalTypedConditionBlock(cfgTyped.set);
  const bool resetCondition = evalTypedConditionBlock(cfgTyped.reset);
  gCardSetResult[cardId] = setCondition;
  gCardResetResult[cardId] = resetCondition;
  gCardResetOverride[cardId] = setCondition && resetCondition;

  V3DoRuntimeConfig cfg = {};
  cfg.mode = cfgTyped.mode;
  cfg.delayBeforeOnMs = cfgTyped.delayBeforeOnMs;
  cfg.onDurationMs = cfgTyped.onDurationMs;
  cfg.repeatCount = cfgTyped.repeatCount;

  V3DoStepInput in = {};
  in.nowMs = nowMs;
  in.setCondition = setCondition;
  in.resetCondition = resetCondition;

  V3DoStepOutput out = {};
  runV3DoStep(cfg, *runtime, in, out);

  mirrorRuntimeStoreCardToLegacyByTyped(card, *cfgCard, gRuntimeStore);
  uint8_t hwPin = 255;
  if (cfgTyped.channel < NUM_DO) hwPin = DO_Pins[cfgTyped.channel];
  if (driveHardware && hwPin != 255 && !isOutputMasked(cardId)) {
    digitalWrite(hwPin, out.effectiveOutput ? HIGH : LOW);
  }
}

void processSIOCard(uint8_t cardId, uint32_t nowMs) {
  if (cardId >= TOTAL_CARDS) return;
  const V3CardConfig* cfgCard = activeTypedCardConfig(cardId);
  if (cfgCard == nullptr || cfgCard->family != V3CardFamily::SIO) return;
  const V3SioConfig& cfgTyped = cfgCard->sio;
  LogicCard& card = logicCards[cardId];
  if (cardId < SIO_START) return;
  V3SioRuntimeState* runtime =
      runtimeSioStateAt(static_cast<uint8_t>(cardId - SIO_START), gRuntimeStore);
  if (runtime == nullptr) return;

  const bool setCondition = evalTypedConditionBlock(cfgTyped.set);
  const bool resetCondition = evalTypedConditionBlock(cfgTyped.reset);
  gCardSetResult[cardId] = setCondition;
  gCardResetResult[cardId] = resetCondition;
  gCardResetOverride[cardId] = setCondition && resetCondition;

  V3SioRuntimeConfig cfg = {};
  cfg.mode = cfgTyped.mode;
  cfg.delayBeforeOnMs = cfgTyped.delayBeforeOnMs;
  cfg.onDurationMs = cfgTyped.onDurationMs;
  cfg.repeatCount = cfgTyped.repeatCount;

  V3SioStepInput in = {};
  in.nowMs = nowMs;
  in.setCondition = setCondition;
  in.resetCondition = resetCondition;

  V3SioStepOutput out = {};
  runV3SioStep(cfg, *runtime, in, out);

  mirrorRuntimeStoreCardToLegacyByTyped(card, *cfgCard, gRuntimeStore);
}

void processMathCard(uint8_t cardId) {
  if (cardId >= TOTAL_CARDS) return;
  const V3CardConfig* cfgCard = activeTypedCardConfig(cardId);
  if (cfgCard == nullptr || cfgCard->family != V3CardFamily::MATH) return;
  const V3MathConfig& cfgTyped = cfgCard->math;
  LogicCard& card = logicCards[cardId];
  if (cardId < MATH_START) return;
  V3MathRuntimeState* runtime = runtimeMathStateAt(
      static_cast<uint8_t>(cardId - MATH_START), gRuntimeStore);
  if (runtime == nullptr) return;

  const bool setCondition = evalTypedConditionBlock(cfgTyped.set);
  const bool resetCondition = evalTypedConditionBlock(cfgTyped.reset);
  gCardSetResult[cardId] = setCondition;
  gCardResetResult[cardId] = resetCondition;
  gCardResetOverride[cardId] = setCondition && resetCondition;

  V3MathRuntimeConfig cfg = {};
  cfg.inputA = cfgTyped.inputA;
  cfg.inputB = cfgTyped.inputB;
  cfg.fallbackValue = cfgTyped.fallbackValue;
  cfg.clampMin = cfgTyped.clampMin;
  cfg.clampMax = cfgTyped.clampMax;
  cfg.clampEnabled = (cfgTyped.clampMax >= cfgTyped.clampMin);

  V3MathStepInput in = {};
  in.setCondition = setCondition;
  in.resetCondition = resetCondition;

  V3MathStepOutput out = {};
  runV3MathStep(cfg, *runtime, in, out);

  mirrorRuntimeStoreCardToLegacyByTyped(card, *cfgCard, gRuntimeStore);
}

void processRtcCard(uint8_t cardId, uint32_t nowMs) {
  if (cardId >= TOTAL_CARDS) return;
  const V3CardConfig* cfgCard = activeTypedCardConfig(cardId);
  if (cfgCard == nullptr || cfgCard->family != V3CardFamily::RTC) return;
  const V3RtcConfig& cfgTyped = cfgCard->rtc;
  LogicCard& card = logicCards[cardId];
  if (cardId < RTC_START) return;
  V3RtcRuntimeState* runtime =
      runtimeRtcStateAt(static_cast<uint8_t>(cardId - RTC_START), gRuntimeStore);
  if (runtime == nullptr) return;
  gCardSetResult[cardId] = false;
  gCardResetResult[cardId] = false;
  gCardResetOverride[cardId] = false;

  V3RtcRuntimeConfig cfg = {};
  cfg.triggerDurationMs = cfgTyped.triggerDurationMs;

  V3RtcStepInput in = {};
  in.nowMs = nowMs;

  runV3RtcStep(cfg, *runtime, in);

  mirrorRuntimeStoreCardToLegacyByTyped(card, *cfgCard, gRuntimeStore);
}

void processCardById(uint8_t cardId, uint32_t nowMs) {
  if (cardId >= TOTAL_CARDS) return;
  const V3CardConfig* cfgCard = activeTypedCardConfig(cardId);
  if (cfgCard == nullptr) return;
  switch (cfgCard->family) {
    case V3CardFamily::DI:
      processDICard(cardId, nowMs);
      return;
    case V3CardFamily::AI:
      processAICard(cardId);
      return;
    case V3CardFamily::SIO:
      processSIOCard(cardId, nowMs);
      return;
    case V3CardFamily::DO:
      processDOCard(cardId, nowMs, true);
      return;
    case V3CardFamily::MATH:
      processMathCard(cardId);
      return;
    case V3CardFamily::RTC:
      processRtcCard(cardId, nowMs);
      return;
    default:
      return;
  }
}

void processOneScanOrderedCard(uint32_t nowMs, bool honorBreakpoints) {
  uint8_t cardId = scanOrderCardIdFromCursor(gScanCursor);
  processCardById(cardId, nowMs);
  refreshRuntimeSignalAt(gRuntimeCardMeta, gRuntimeStore, gRuntimeSignals,
                         TOTAL_CARDS, cardId);
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

void serviceRtcMinuteScheduler(uint32_t nowMs) {
  if (!gRtcClockInitialized) return;

  static uint32_t lastPollMs = 0;
  if ((nowMs - lastPollMs) < 200) return;
  lastPollMs = nowMs;

  DateTime now = gRtcClock.now();
  V3RtcMinuteStamp stamp = {};
  stamp.year = now.year();
  stamp.month = now.month();
  stamp.day = now.day();
  stamp.weekday = now.dayOfTheWeek();
  stamp.hour = now.hour();
  stamp.minute = now.minute();

  const int32_t minuteKey = v3RtcMinuteKey(stamp);
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
    V3RtcScheduleView view = {};
    view.enabled = channel.enabled;
    view.year = channel.year;
    view.month = channel.month;
    view.day = channel.day;
    view.weekday = channel.weekday;
    view.hour = channel.hour;
    view.minute = channel.minute;
    view.rtcCardId = channel.rtcCardId;
    if (!v3RtcChannelMatchesMinute(view, stamp)) continue;
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
