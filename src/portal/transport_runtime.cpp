/*
File: src/portal/transport_runtime.cpp
Purpose: Implements the transport runtime module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/main.cpp
Flow Hook:
- Portal request handling and runtime snapshot transport.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "portal/transport_runtime.h"

#include <Arduino.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <time.h>

#include <LittleFS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <uri/UriBraces.h>

#include "kernel/enum_codec.h"
#include "portal/transport_command_stub.h"
#include "storage/v3_config_decoder.h"
#include "storage/v3_config_validator.h"

namespace v3::portal {

namespace {

WebServer gHttpServer(80);
WebSocketsServer gWsServer(81);
PortalService* gPortal = nullptr;
v3::storage::StorageService* gStorage = nullptr;
bool gTransportInitialized = false;
uint32_t gLastTransportActivityMs = 0;
constexpr bool kLogHttpVerboseReads = false;
constexpr bool kLogHttpPageNavigation = true;
constexpr bool kLogHttpMutations = false;
constexpr bool kLogHttpActions = false;
constexpr bool kLogTransport404 = false;
constexpr bool kLogClientConnections = true;
constexpr bool kLogSettingsCommit = true;
constexpr bool kLogTransportTiming = true;
constexpr uint32_t kSlowTransportLogMs = 100;

void sendNoContent() { gHttpServer.send(204, "text/plain", ""); }
void markTransportActivity() { gLastTransportActivityMs = millis(); }
void feedTaskWatchdog() { (void)esp_task_wdt_reset(); }

void logTransportTiming(const char* tag, uint32_t elapsedMs) {
  if (!kLogTransportTiming) return;
  if (elapsedMs < kSlowTransportLogMs) return;
  Serial.printf("[transport:slow] %s elapsed=%lums heap=%lu\n", tag,
                static_cast<unsigned long>(elapsedMs),
                static_cast<unsigned long>(ESP.getFreeHeap()));
  Serial.flush();
}

const char* contentTypeForPath(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool sendLittleFsFile(const char* path) {
  const uint32_t startedMs = millis();
  feedTaskWatchdog();
  if (!LittleFS.exists(path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  if (kLogTransportTiming) {
    Serial.printf("[transport:file] start path=%s size=%lu heap=%lu\n", path,
                  static_cast<unsigned long>(file.size()),
                  static_cast<unsigned long>(ESP.getFreeHeap()));
    Serial.flush();
  }
  gHttpServer.streamFile(file, contentTypeForPath(String(path)));
  feedTaskWatchdog();
  file.close();
  const uint32_t elapsedMs = millis() - startedMs;
  if (kLogTransportTiming) {
    Serial.printf("[transport:file] done path=%s elapsed=%lums heap=%lu\n", path,
                  static_cast<unsigned long>(elapsedMs),
                  static_cast<unsigned long>(ESP.getFreeHeap()));
    Serial.flush();
  }
  return true;
}

const char* methodToString(const HTTPMethod method) {
  switch (method) {
    case HTTP_GET:
      return "GET";
    case HTTP_POST:
      return "POST";
    case HTTP_PUT:
      return "PUT";
    case HTTP_PATCH:
      return "PATCH";
    case HTTP_DELETE:
      return "DELETE";
    case HTTP_OPTIONS:
      return "OPTIONS";
    default:
      return "UNKNOWN";
  }
}

void handleHttpCorsOptions() {
  gHttpServer.sendHeader("Access-Control-Allow-Origin", "*");
  gHttpServer.sendHeader("Access-Control-Allow-Methods",
                         "GET,POST,PUT,PATCH,DELETE,OPTIONS,HEAD");
  gHttpServer.sendHeader("Access-Control-Allow-Headers", "*");
  gHttpServer.sendHeader("Access-Control-Max-Age", "600");
  sendNoContent();
}

const char* familyToString(const v3::storage::CardFamily family) {
  switch (family) {
    case v3::storage::CardFamily::DI:
      return "DI";
    case v3::storage::CardFamily::DO:
      return "DO";
    case v3::storage::CardFamily::AI:
      return "AI";
    case v3::storage::CardFamily::SIO:
      return "SIO";
    case v3::storage::CardFamily::MATH:
      return "MATH";
    case v3::storage::CardFamily::RTC:
      return "RTC";
    default:
      return "UNKNOWN";
  }
}

const char* conditionOperatorToString(const v3::storage::ConditionOperator op) {
  switch (op) {
    case v3::storage::ConditionOperator::AlwaysTrue:
      return "ALWAYS_TRUE";
    case v3::storage::ConditionOperator::AlwaysFalse:
      return "ALWAYS_FALSE";
    case v3::storage::ConditionOperator::LogicalTrue:
      return "LOGICAL_TRUE";
    case v3::storage::ConditionOperator::LogicalFalse:
      return "LOGICAL_FALSE";
    case v3::storage::ConditionOperator::PhysicalOn:
      return "PHYSICAL_ON";
    case v3::storage::ConditionOperator::PhysicalOff:
      return "PHYSICAL_OFF";
    case v3::storage::ConditionOperator::Triggered:
      return "TRIGGERED";
    case v3::storage::ConditionOperator::TriggerCleared:
      return "TRIGGER_CLEARED";
    case v3::storage::ConditionOperator::GT:
      return "GT";
    case v3::storage::ConditionOperator::LT:
      return "LT";
    case v3::storage::ConditionOperator::EQ:
      return "EQ";
    case v3::storage::ConditionOperator::NEQ:
      return "NEQ";
    case v3::storage::ConditionOperator::GTE:
      return "GTE";
    case v3::storage::ConditionOperator::LTE:
      return "LTE";
    case v3::storage::ConditionOperator::Running:
      return "RUNNING";
    case v3::storage::ConditionOperator::Finished:
      return "FINISHED";
    case v3::storage::ConditionOperator::Stopped:
      return "STOPPED";
    default:
      return "ALWAYS_FALSE";
  }
}

const char* conditionCombinerToString(const v3::storage::ConditionCombiner combiner) {
  switch (combiner) {
    case v3::storage::ConditionCombiner::None:
      return "NONE";
    case v3::storage::ConditionCombiner::And:
      return "AND";
    case v3::storage::ConditionCombiner::Or:
      return "OR";
    default:
      return "NONE";
  }
}

void writeConditionClause(JsonObject dst, const v3::storage::ConditionClause& clause) {
  dst["sourceCardId"] = clause.sourceCardId;
  dst["operator"] = conditionOperatorToString(clause.op);
  dst["thresholdValue"] = clause.thresholdValue;
  dst["thresholdCardId"] = clause.thresholdCardId;
  dst["useThresholdCard"] = clause.useThresholdCard;
}

void writeConditionBlock(JsonObject dst, const v3::storage::ConditionBlock& block) {
  dst["combiner"] = conditionCombinerToString(block.combiner);
  JsonObject clauseA = dst["clauseA"].to<JsonObject>();
  writeConditionClause(clauseA, block.clauseA);
  // Keep payload shape stable for all cards by always emitting clauseB.
  JsonObject clauseB = dst["clauseB"].to<JsonObject>();
  writeConditionClause(clauseB, block.clauseB);
}

void writeSettingsJson(JsonObject out, const v3::storage::SystemConfig& cfg) {
  out["schemaVersion"] = cfg.schemaVersion;
  out["scanPeriodMs"] = cfg.scanPeriodMs;

  JsonObject wifi = out["wifi"].to<JsonObject>();
  JsonObject backup = wifi["backupAccessNetwork"].to<JsonObject>();
  backup["ssid"] = cfg.wifi.backupAccessNetwork.ssid;
  backup["password"] = cfg.wifi.backupAccessNetwork.password;
  backup["timeoutSec"] = cfg.wifi.backupAccessNetwork.timeoutSec;
  backup["editable"] = cfg.wifi.backupAccessNetwork.editable;

  JsonObject user = wifi["userConfiguredNetwork"].to<JsonObject>();
  user["ssid"] = cfg.wifi.userConfiguredNetwork.ssid;
  user["password"] = cfg.wifi.userConfiguredNetwork.password;
  user["timeoutSec"] = cfg.wifi.userConfiguredNetwork.timeoutSec;
  user["editable"] = cfg.wifi.userConfiguredNetwork.editable;

  wifi["retryDelaySec"] = cfg.wifi.retryDelaySec;
  wifi["staOnly"] = cfg.wifi.staOnly;

  JsonObject time = out["time"].to<JsonObject>();
  time["timezone"] = cfg.time.timezone;
  JsonObject timeSync = time["timeSync"].to<JsonObject>();
  timeSync["enabled"] = cfg.time.timeSync.enabled;
  timeSync["primaryTimeServer"] = cfg.time.timeSync.primaryTimeServer;
  timeSync["syncIntervalSec"] = cfg.time.timeSync.syncIntervalSec;
  timeSync["startupTimeoutSec"] = cfg.time.timeSync.startupTimeoutSec;
  timeSync["maxTimeAgeSec"] = cfg.time.timeSync.maxTimeAgeSec;
}

void writeCardJson(JsonObject out, const v3::storage::CardConfig& card) {
  out["id"] = card.id;
  out["family"] = familyToString(card.family);
  out["enabled"] = card.enabled;
  JsonObject params = out["params"].to<JsonObject>();
  switch (card.family) {
    case v3::storage::CardFamily::DI:
      params["channel"] = card.di.channel;
      params["invert"] = card.di.invert;
      params["debounceMs"] = card.di.debounceMs;
      params["edgeMode"] = card.di.edgeMode;
      writeConditionBlock(params["turnOnCondition"].to<JsonObject>(),
                          card.di.turnOnCondition);
      writeConditionBlock(params["turnOffCondition"].to<JsonObject>(),
                          card.di.turnOffCondition);
      break;
    case v3::storage::CardFamily::DO:
      params["channel"] = card.dout.channel;
      params["invert"] = card.dout.invert;
      params["mode"] = card.dout.mode;
      params["delayBeforeOnMs"] = card.dout.delayBeforeOnMs;
      params["activeDurationMs"] = card.dout.activeDurationMs;
      params["repeatCount"] = card.dout.repeatCount;
      writeConditionBlock(params["turnOnCondition"].to<JsonObject>(),
                          card.dout.turnOnCondition);
      writeConditionBlock(params["turnOffCondition"].to<JsonObject>(),
                          card.dout.turnOffCondition);
      break;
    case v3::storage::CardFamily::AI:
      params["channel"] = card.ai.channel;
      params["inputMin"] = card.ai.inputMin;
      params["inputMax"] = card.ai.inputMax;
      params["outputMin"] = card.ai.outputMin;
      params["outputMax"] = card.ai.outputMax;
      params["smoothingFactorPct"] = card.ai.smoothingFactorPct;
      break;
    case v3::storage::CardFamily::SIO:
      params["invert"] = card.sio.invert;
      params["mode"] = card.sio.mode;
      params["delayBeforeOnMs"] = card.sio.delayBeforeOnMs;
      params["activeDurationMs"] = card.sio.activeDurationMs;
      params["repeatCount"] = card.sio.repeatCount;
      writeConditionBlock(params["turnOnCondition"].to<JsonObject>(),
                          card.sio.turnOnCondition);
      writeConditionBlock(params["turnOffCondition"].to<JsonObject>(),
                          card.sio.turnOffCondition);
      break;
    case v3::storage::CardFamily::MATH:
      params["operation"] = card.math.operation;
      params["inputAUseCard"] = card.math.inputAUseCard;
      params["inputACardId"] = card.math.inputACardId;
      params["inputA"] = card.math.inputA;
      params["inputBUseCard"] = card.math.inputBUseCard;
      params["inputBCardId"] = card.math.inputBCardId;
      params["inputB"] = card.math.inputB;
      params["inputMin"] = card.math.inputMin;
      params["inputMax"] = card.math.inputMax;
      params["outputMin"] = card.math.outputMin;
      params["outputMax"] = card.math.outputMax;
      params["smoothingFactorPct"] = card.math.smoothingFactorPct;
      params["fallbackValue"] = card.math.fallbackValue;
      writeConditionBlock(params["turnOnCondition"].to<JsonObject>(),
                          card.math.turnOnCondition);
      writeConditionBlock(params["turnOffCondition"].to<JsonObject>(),
                          card.math.turnOffCondition);
      break;
    case v3::storage::CardFamily::RTC:
      params["hasYear"] = card.rtc.hasYear;
      params["year"] = card.rtc.year;
      params["hasMonth"] = card.rtc.hasMonth;
      params["month"] = card.rtc.month;
      params["hasDay"] = card.rtc.hasDay;
      params["day"] = card.rtc.day;
      params["hasWeekday"] = card.rtc.hasWeekday;
      params["weekday"] = card.rtc.weekday;
      params["hasHour"] = card.rtc.hasHour;
      params["hour"] = card.rtc.hour;
      params["minute"] = card.rtc.minute;
      params["triggerDurationMs"] = card.rtc.triggerDurationMs;
      break;
    default:
      break;
  }
}

void logHttpAccess(const char* routeTag) {
  const bool isGet = (gHttpServer.method() == HTTP_GET);
  const bool isPageRoute =
      (routeTag != nullptr) && (strncmp(routeTag, "page.", 5) == 0);
  if (!isGet && !kLogHttpMutations) return;
  if (isGet && !kLogHttpVerboseReads) {
    if (!(kLogHttpPageNavigation && isPageRoute)) return;
  }
  const IPAddress remoteIp = gHttpServer.client().remoteIP();
  const String uri = gHttpServer.uri();
  Serial.printf("[http] %s %s from %u.%u.%u.%u route=%s\n",
                methodToString(gHttpServer.method()), uri.c_str(), remoteIp[0],
                remoteIp[1], remoteIp[2], remoteIp[3], routeTag);
  Serial.flush();
}

void writeRuntimeCardJson(JsonObject item, const RuntimeSnapshotCard& card) {
  item["id"] = card.id;
  item["type"] = toString(card.type);
  item["index"] = card.index;
  item["commandState"] = card.commandState;
  item["actualState"] = card.actualState;
  item["edgePulse"] = card.edgePulse;
  item["state"] = toString(card.state);
  item["mode"] = toString(card.mode);
  item["liveValue"] = card.liveValue;
  item["startOnMs"] = card.startOnMs;
  item["startOffMs"] = card.startOffMs;
  item["repeatCounter"] = card.repeatCounter;
  item["turnOnConditionMet"] = card.turnOnConditionMet;
  item["turnOffConditionMet"] = card.turnOffConditionMet;
}

void writeRuntimeClockJson(JsonObject out) {
  const time_t epochNow = time(nullptr);
  struct tm localTime = {};
  if (epochNow <= 0 || localtime_r(&epochNow, &localTime) == nullptr) {
    out["valid"] = false;
    out["epochSec"] = 0;
    return;
  }

  static const char* kWeekdayShort[7] = {"Sun", "Mon", "Tue", "Wed",
                                         "Thu", "Fri", "Sat"};
  static const char* kMonthShort[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  const int year = localTime.tm_year + 1900;
  const int month = localTime.tm_mon + 1;
  const int day = localTime.tm_mday;
  const int weekday = localTime.tm_wday;
  const int hour24 = localTime.tm_hour;
  const int minute = localTime.tm_min;
  const int second = localTime.tm_sec;
  const bool isPm = (hour24 >= 12);
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;

  char formatted24[16] = {};
  char formatted12[20] = {};
  std::snprintf(formatted24, sizeof(formatted24), "%02d:%02d:%02d", hour24,
                minute, second);
  std::snprintf(formatted12, sizeof(formatted12), "%02d:%02d:%02d %s", hour12,
                minute, second, isPm ? "PM" : "AM");

  out["valid"] = true;
  out["epochSec"] = static_cast<uint32_t>(epochNow);
  out["year"] = year;
  out["month"] = month;
  out["monthName"] =
      (month >= 1 && month <= 12) ? kMonthShort[month - 1] : "?";
  out["day"] = day;
  out["weekday"] = weekday;
  out["weekdayName"] =
      (weekday >= 0 && weekday <= 6) ? kWeekdayShort[weekday] : "?";
  out["hour24"] = hour24;
  out["hour12"] = hour12;
  out["minute"] = minute;
  out["second"] = second;
  out["amPm"] = isPm ? "PM" : "AM";
  out["formatted24"] = formatted24;
  out["formatted12"] = formatted12;
}

int8_t findCardIndexById(const v3::storage::SystemConfig& cfg, uint8_t cardId) {
  for (uint8_t i = 0; i < cfg.cardCount; ++i) {
    if (cfg.cards[i].id == cardId) return static_cast<int8_t>(i);
  }
  return -1;
}

bool parseCardIdFromPathArg(uint8_t& outCardId) {
  const String idArg = gHttpServer.pathArg(0);
  if (idArg.length() == 0) return false;
  for (size_t i = 0; i < idArg.length(); ++i) {
    const char c = idArg.charAt(i);
    if (c < '0' || c > '9') return false;
  }
  const unsigned long value = strtoul(idArg.c_str(), nullptr, 10);
  if (value > 255UL) return false;
  outCardId = static_cast<uint8_t>(value);
  return true;
}

void sendConfigError(int statusCode, const char* errorCode, const String& requestId,
                     const String& message) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["apiVersion"] = "2.0";
  doc["status"] = "ERROR";
  if (requestId.length() > 0) doc["requestId"] = requestId;
  doc["tsMs"] = millis();
  JsonObject error = doc["error"].to<JsonObject>();
  error["errorCode"] = errorCode;
  error["message"] = message;
  String body;
  serializeJson(doc, body);
  gHttpServer.send(statusCode, "application/json", body);
}

/**
 * @brief Handles HTTP command submit endpoint.
 * @details Delegates payload parsing/execution to transport command stub and
 * writes JSON response.
 * @par Used By
 * `POST /api/v3/command`.
 */
void handleHttpCommandSubmit() {
  markTransportActivity();
  logHttpAccess("api.command.submit");
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }

  const String payload = gHttpServer.arg("plain");
  const TransportCommandResponse response = handleTransportCommandStub(
      *gPortal, payload.c_str(), TransportCommandSource::Http, micros());
  gHttpServer.send(response.statusCode, "application/json", response.body);
}

void handleHttpRuntimeMetricsGet() {
  markTransportActivity();
  logHttpAccess("api.runtime.metrics.get");
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }
  const PortalSnapshotState state = gPortal->snapshotState();
  if (!state.ready) {
    gHttpServer.send(503, "application/json",
                     "{\"ok\":false,\"reason\":\"snapshot_not_ready\"}");
    return;
  }
  const v3::runtime::RuntimeSnapshot& snapshot =
      gPortal->latestRuntimeSnapshot();

  JsonDocument out;
  out["ok"] = true;
  out["apiVersion"] = "2.0";
  out["status"] = "SUCCESS";
  out["tsMs"] = snapshot.nowMs;
  JsonObject runtime = out["runtime"].to<JsonObject>();
  runtime["snapshotSeq"] = snapshot.completedScans;
  runtime["engineMode"] = toString(snapshot.mode);
  runtime["scanPeriodMs"] = snapshot.scanPeriodMs;
  runtime["cardCount"] = snapshot.enabledCardCount;
  JsonObject clock = runtime["clock"].to<JsonObject>();
  writeRuntimeClockJson(clock);
  JsonObject metrics = runtime["metrics"].to<JsonObject>();
  metrics["scanLastUs"] = snapshot.scanLastUs;
  metrics["scanMaxUs"] = snapshot.scanMaxUs;
  metrics["scanBudgetUs"] = snapshot.scanPeriodMs * 1000U;
  metrics["scanOverrunLast"] = snapshot.scanOverrunLast;
  metrics["scanOverrunCount"] = snapshot.scanOverrunCount;
  metrics["queueDepth"] = snapshot.queueTelemetry.commandQueueDepth;
  metrics["queueHighWaterMark"] = snapshot.queueTelemetry.commandQueueHighWater;
  metrics["queueCapacity"] = snapshot.queueTelemetry.commandQueueCapacity;
  metrics["commandLatencyLastUs"] = snapshot.queueTelemetry.commandLastLatencyUs;
  metrics["commandLatencyMaxUs"] = snapshot.queueTelemetry.commandMaxLatencyUs;
  metrics["configuredCardCount"] = snapshot.configuredCardCount;
  metrics["enabledCardCount"] = snapshot.enabledCardCount;
  metrics["snapshotQueueDepth"] = snapshot.queueTelemetry.snapshotQueueDepth;
  metrics["snapshotQueueHighWater"] = snapshot.queueTelemetry.snapshotQueueHighWater;
  metrics["snapshotQueueDropCount"] = snapshot.queueTelemetry.snapshotQueueDropCount;
  metrics["minFreeHeapBytes"] = snapshot.memory.minFreeHeapBytes;
  metrics["minLargestFreeBlockBytes"] = snapshot.memory.minLargestFreeBlockBytes;
  metrics["core0StackHighWaterBytes"] = snapshot.memory.core0StackHighWaterBytes;
  metrics["core1StackHighWaterBytes"] = snapshot.memory.core1StackHighWaterBytes;

  String body;
  serializeJson(out, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpRuntimeCardsGet() {
  markTransportActivity();
  logHttpAccess("api.runtime.cards.get");
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }
  const PortalSnapshotState state = gPortal->snapshotState();
  if (!state.ready) {
    gHttpServer.send(503, "application/json",
                     "{\"ok\":false,\"reason\":\"snapshot_not_ready\"}");
    return;
  }
  const v3::runtime::RuntimeSnapshot& snapshot =
      gPortal->latestRuntimeSnapshot();
  uint8_t cardCount = 0;
  const RuntimeSnapshotCard* cards = gPortal->latestRuntimeCards(cardCount);

  JsonDocument out;
  out["ok"] = true;
  out["apiVersion"] = "2.0";
  out["status"] = "SUCCESS";
  out["tsMs"] = snapshot.nowMs;
  out["snapshotSeq"] = snapshot.completedScans;
  JsonArray cardsJson = out["cards"].to<JsonArray>();
  for (uint8_t i = 0; i < cardCount; ++i) {
    JsonObject item = cardsJson.add<JsonObject>();
    writeRuntimeCardJson(item, cards[i]);
  }
  String body;
  serializeJson(out, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpRuntimeCardsDeltaGet() {
  markTransportActivity();
  logHttpAccess("api.runtime.cards.delta.get");
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }
  const PortalSnapshotState state = gPortal->snapshotState();
  if (!state.ready) {
    gHttpServer.send(503, "application/json",
                     "{\"ok\":false,\"reason\":\"snapshot_not_ready\"}");
    return;
  }

  uint32_t sinceSeq = 0;
  const String sinceRaw = gHttpServer.arg("since");
  if (sinceRaw.length() > 0) {
    for (size_t i = 0; i < sinceRaw.length(); ++i) {
      const char c = sinceRaw.charAt(i);
      if (c < '0' || c > '9') {
        gHttpServer.send(400, "application/json",
                         "{\"ok\":false,\"reason\":\"invalid_since\"}");
        return;
      }
    }
    sinceSeq = strtoul(sinceRaw.c_str(), nullptr, 10);
  }

  const v3::runtime::RuntimeSnapshot& snapshot =
      gPortal->latestRuntimeSnapshot();
  const uint32_t latestSeq = snapshot.completedScans;
  const uint32_t previousSeq = gPortal->previousRuntimeSnapshotSeq();
  uint8_t cardCount = 0;
  const RuntimeSnapshotCard* cards = gPortal->latestRuntimeCards(cardCount);

  bool resyncRequired = false;
  bool emitAllCards = false;
  if (sinceSeq == latestSeq) {
    emitAllCards = false;
  } else if (sinceSeq == previousSeq) {
    emitAllCards = false;
  } else {
    resyncRequired = true;
    emitAllCards = true;
  }

  JsonDocument out;
  out["ok"] = true;
  out["apiVersion"] = "2.0";
  out["status"] = "SUCCESS";
  out["tsMs"] = snapshot.nowMs;
  out["sinceSeq"] = sinceSeq;
  out["previousSeq"] = previousSeq;
  out["snapshotSeq"] = latestSeq;
  out["resyncRequired"] = resyncRequired;
  JsonArray cardsJson = out["cards"].to<JsonArray>();
  for (uint8_t i = 0; i < cardCount; ++i) {
    if (!emitAllCards && sinceSeq == previousSeq &&
        !gPortal->isRuntimeCardChangedSincePrevious(i)) {
      continue;
    }
    JsonObject item = cardsJson.add<JsonObject>();
    writeRuntimeCardJson(item, cards[i]);
  }
  String body;
  serializeJson(out, body);
  gHttpServer.send(200, "application/json", body);
}

/**
 * @brief Handles HTTP diagnostics endpoint.
 * @par Used By
 * `GET /api/v3/diagnostics`.
 */
void handleHttpDiagnosticsGet() {
  markTransportActivity();
  logHttpAccess("api.diagnostics.get");
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }
  const PortalDiagnosticsState state = gPortal->diagnosticsState();
  if (!state.ready || state.json == nullptr || state.json[0] == '\0') {
    gHttpServer.send(503, "application/json",
                     "{\"ok\":false,\"reason\":\"diagnostics_not_ready\"}");
    return;
  }
  gHttpServer.send(200, "application/json", state.json);
}

void handleHttpConfigRestorePost() {
  markTransportActivity();
  logHttpAccess("api.config.restore.post");
  if (gStorage == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"storage_not_ready\"}");
    return;
  }
  const String payload = gHttpServer.arg("plain");
  JsonDocument requestDoc;
  const DeserializationError jsonErr = deserializeJson(requestDoc, payload);
  if (jsonErr) {
    sendConfigError(400, "INVALID_REQUEST", "", "invalid JSON payload");
    return;
  }
  const JsonObjectConst root = requestDoc.as<JsonObjectConst>();
  String requestId =
      root["requestId"].is<const char*>() ? String(root["requestId"].as<const char*>())
                                          : String();
  const char* source = root["source"].as<const char*>();
  if (source == nullptr) {
    sendConfigError(400, "INVALID_REQUEST", requestId, "source is required");
    return;
  }
  if (kLogHttpActions) {
    Serial.printf("[http-action] config.restore requestId=%s source=%s\n",
                  requestId.c_str(), source);
    Serial.flush();
  }
  bool restored = false;
  if (strcmp(source, "FACTORY") == 0) {
    restored = gStorage->restoreFactory();
  } else {
    sendConfigError(422, "RESTORE_FAILED", requestId,
                    "unsupported source: only FACTORY");
    return;
  }
  if (!restored) {
    sendConfigError(409, "RESTORE_FAILED", requestId, "restore operation failed");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["requestId"] = requestId;
  doc["tsMs"] = millis();
  doc["restoredFrom"] = source;
  doc["activeVersion"] = gStorage->activeRevision();
  doc["requiresRestart"] = true;
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpSettingsGet() {
  markTransportActivity();
  logHttpAccess("api.settings.get");
  const uint32_t startedMs = millis();
  if (kLogTransportTiming) {
    Serial.printf("[settings.get] start heap=%lu\n",
                  static_cast<unsigned long>(ESP.getFreeHeap()));
    Serial.flush();
  }
  if (gStorage == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"storage_not_ready\"}");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["tsMs"] = millis();
  doc["activeVersion"] = gStorage->activeRevision();
  JsonObject settings = doc["settings"].to<JsonObject>();
  writeSettingsJson(settings, gStorage->activeSystemConfig());
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
  feedTaskWatchdog();
  const uint32_t elapsedMs = millis() - startedMs;
  if (kLogTransportTiming) {
    Serial.printf("[settings.get] done elapsed=%lums bytes=%u heap=%lu\n",
                  static_cast<unsigned long>(elapsedMs),
                  static_cast<unsigned>(body.length()),
                  static_cast<unsigned long>(ESP.getFreeHeap()));
    Serial.flush();
  }
}

void handleHttpSettingsPut() {
  markTransportActivity();
  logHttpAccess("api.settings.put");
  if (gStorage == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"storage_not_ready\"}");
    return;
  }
  const String payload = gHttpServer.arg("plain");
  JsonDocument requestDoc;
  const DeserializationError jsonErr = deserializeJson(requestDoc, payload);
  if (jsonErr) {
    sendConfigError(400, "INVALID_REQUEST", "", "invalid JSON payload");
    return;
  }

  JsonObjectConst root = requestDoc.as<JsonObjectConst>();
  JsonObjectConst settingsObj = root["settings"].as<JsonObjectConst>();
  if (settingsObj.isNull()) settingsObj = root;
  String requestId = root["requestId"].is<const char*>()
                         ? String(root["requestId"].as<const char*>())
                         : String();
  if (kLogHttpActions || kLogSettingsCommit) {
    Serial.printf("[settings.put] requestId=%s payloadBytes=%u\n", requestId.c_str(),
                  static_cast<unsigned>(payload.length()));
    Serial.flush();
  }

  v3::storage::SystemConfig* candidate = gStorage->prepareStagedFromActive();
  if (candidate == nullptr) {
    if (kLogSettingsCommit) {
      Serial.printf("[settings.put] requestId=%s staged copy alloc failed\n",
                    requestId.c_str());
      Serial.flush();
    }
    sendConfigError(503, "INSUFFICIENT_MEMORY", requestId,
                    "settings staging buffer unavailable");
    return;
  }
  v3::storage::ConfigValidationError decodeError = {
      v3::storage::ConfigErrorCode::None, 0};
  if (!v3::storage::decodeSystemSettingsLight(settingsObj, *candidate,
                                              decodeError)) {
    if (kLogSettingsCommit) {
      Serial.printf("[settings.put] requestId=%s decode failed code=%s\n",
                    requestId.c_str(),
                    v3::storage::configErrorCodeToString(decodeError.code));
      Serial.flush();
    }
    sendConfigError(422, "VALIDATION_FAILED", requestId,
                    v3::storage::configErrorCodeToString(decodeError.code));
    return;
  }
  v3::storage::ConfigValidationError validationError = {
      v3::storage::ConfigErrorCode::None, 0};
  if (!v3::storage::validateSystemConfigLight(*candidate, validationError)) {
    if (kLogSettingsCommit) {
      Serial.printf("[settings.put] requestId=%s validate failed code=%s\n",
                    requestId.c_str(),
                    v3::storage::configErrorCodeToString(validationError.code));
      Serial.flush();
    }
    sendConfigError(422, "VALIDATION_FAILED", requestId,
                    v3::storage::configErrorCodeToString(validationError.code));
    return;
  }

  if (!gStorage->stageSystemConfig(*candidate)) {
    if (kLogSettingsCommit) {
      Serial.printf("[settings.put] requestId=%s stage failed\n", requestId.c_str());
      Serial.flush();
    }
    sendConfigError(503, "INSUFFICIENT_MEMORY", requestId,
                    "settings staging buffer unavailable");
    return;
  }
  if (kLogSettingsCommit) {
    Serial.printf("[settings.put] requestId=%s commit start\n", requestId.c_str());
    Serial.flush();
  }
  if (!gStorage->commitStaged()) {
    if (kLogSettingsCommit) {
      Serial.printf("[settings.put] requestId=%s commit failed\n", requestId.c_str());
      Serial.flush();
    }
    sendConfigError(409, "COMMIT_FAILED", requestId, "settings commit failed");
    return;
  }
  if (kLogSettingsCommit) {
    Serial.printf("[settings.put] requestId=%s commit ok activeVersion=%lu\n",
                  requestId.c_str(),
                  static_cast<unsigned long>(gStorage->activeRevision()));
    Serial.flush();
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["requestId"] = requestId;
  doc["tsMs"] = millis();
  doc["activeVersion"] = gStorage->activeRevision();
  doc["requiresRestart"] = true;
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpSystemRebootPost() {
  markTransportActivity();
  logHttpAccess("api.system.reboot.post");
  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["tsMs"] = millis();
  doc["message"] = "rebooting";
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
  delay(200);
  esp_restart();
}

void handleHttpCardsIndexGet() {
  markTransportActivity();
  logHttpAccess("api.cards.index.get");
  if (gStorage == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"storage_not_ready\"}");
    return;
  }
  const v3::storage::SystemConfig& cfg = gStorage->activeSystemConfig();
  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["tsMs"] = millis();
  doc["activeVersion"] = gStorage->activeRevision();
  JsonArray cards = doc["cards"].to<JsonArray>();
  for (uint8_t i = 0; i < cfg.cardCount; ++i) {
    JsonObject item = cards.add<JsonObject>();
    item["id"] = cfg.cards[i].id;
    item["family"] = familyToString(cfg.cards[i].family);
    item["enabled"] = cfg.cards[i].enabled;
  }
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpCardGetById(uint8_t cardId) {
  markTransportActivity();
  logHttpAccess("api.cards.by-id.get");
  if (gStorage == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"storage_not_ready\"}");
    return;
  }
  const v3::storage::SystemConfig& cfg = gStorage->activeSystemConfig();
  const int8_t index = findCardIndexById(cfg, cardId);
  if (index < 0) {
    gHttpServer.send(404, "application/json",
                     "{\"ok\":false,\"reason\":\"card_not_found\"}");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["tsMs"] = millis();
  doc["activeVersion"] = gStorage->activeRevision();
  JsonObject card = doc["card"].to<JsonObject>();
  writeCardJson(card, cfg.cards[static_cast<uint8_t>(index)]);
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpCardPutById(uint8_t cardId) {
  markTransportActivity();
  logHttpAccess("api.cards.by-id.put");
  if (gStorage == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"storage_not_ready\"}");
    return;
  }
  const String payload = gHttpServer.arg("plain");
  JsonDocument requestDoc;
  const DeserializationError jsonErr = deserializeJson(requestDoc, payload);
  if (jsonErr) {
    sendConfigError(400, "INVALID_REQUEST", "", "invalid JSON payload");
    return;
  }

  JsonObjectConst root = requestDoc.as<JsonObjectConst>();
  JsonObjectConst cardObj = root["card"].as<JsonObjectConst>();
  if (cardObj.isNull()) cardObj = root;
  String requestId = root["requestId"].is<const char*>()
                         ? String(root["requestId"].as<const char*>())
                         : String();
  if (kLogHttpActions) {
    Serial.printf("[http-action] card.put id=%u requestId=%s\n",
                  static_cast<unsigned>(cardId), requestId.c_str());
    Serial.flush();
  }

  v3::storage::CardConfig decodedCard = {};
  v3::storage::ConfigValidationError decodeError = {
      v3::storage::ConfigErrorCode::None, 0};
  if (!v3::storage::decodeCardConfigLight(cardObj, decodedCard, decodeError, 0)) {
    sendConfigError(422, "VALIDATION_FAILED", requestId,
                    v3::storage::configErrorCodeToString(decodeError.code));
    return;
  }
  if (decodedCard.id != cardId) {
    sendConfigError(422, "VALIDATION_FAILED", requestId, "card id mismatch");
    return;
  }

  v3::storage::SystemConfig* candidate = gStorage->prepareStagedFromActive();
  if (candidate == nullptr) {
    sendConfigError(503, "INSUFFICIENT_MEMORY", requestId,
                    "card staging buffer unavailable");
    return;
  }
  const int8_t index = findCardIndexById(*candidate, cardId);
  if (index < 0) {
    sendConfigError(404, "CARD_NOT_FOUND", requestId, "card not found");
    return;
  }
  candidate->cards[static_cast<uint8_t>(index)] = decodedCard;

  v3::storage::ConfigValidationError validationError = {
      v3::storage::ConfigErrorCode::None, 0};
  if (!v3::storage::validateSystemConfigLight(*candidate, validationError)) {
    sendConfigError(422, "VALIDATION_FAILED", requestId,
                    v3::storage::configErrorCodeToString(validationError.code));
    return;
  }

  if (!gStorage->stageSystemConfig(*candidate)) {
    sendConfigError(503, "INSUFFICIENT_MEMORY", requestId,
                    "card staging buffer unavailable");
    return;
  }
  if (!gStorage->commitStaged()) {
    sendConfigError(409, "COMMIT_FAILED", requestId, "card commit failed");
    return;
  }

  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["requestId"] = requestId;
  doc["tsMs"] = millis();
  doc["activeVersion"] = gStorage->activeRevision();
  doc["requiresRestart"] = true;
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpCardGetByPathArg() {
  uint8_t cardId = 0;
  if (!parseCardIdFromPathArg(cardId)) {
    gHttpServer.send(400, "application/json",
                     "{\"ok\":false,\"reason\":\"invalid_card_path\"}");
    return;
  }
  handleHttpCardGetById(cardId);
}

void handleHttpCardPutByPathArg() {
  uint8_t cardId = 0;
  if (!parseCardIdFromPathArg(cardId)) {
    gHttpServer.send(400, "application/json",
                     "{\"ok\":false,\"reason\":\"invalid_card_path\"}");
    return;
  }
  handleHttpCardPutById(cardId);
}

/**
 * @brief Handles root landing endpoint.
 * @details Returns a tiny discovery payload with primary API routes so raw-IP
 * browser opens get a valid response.
 * @par Used By
 * `GET /`.
 */
void handleHttpRootGet() {
  markTransportActivity();
  logHttpAccess("page.root");
  if (sendLittleFsFile("/index.html")) {
    return;
  }
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }
  gHttpServer.send(
      200, "application/json",
      "{\"ok\":true,\"service\":\"advanced-timer-v3\",\"routes\":[\"/api/v3/"
      "diagnostics\",\"/api/v3/command\","
      "\"/api/v3/runtime/metrics\",\"/api/v3/runtime/cards\","
      "\"/api/v3/runtime/cards/delta\","
      "\"/api/v3/settings\",\"/api/v3/cards\",\"/api/v3/cards/{id}\","
      "\"/api/v3/config/restore\"]}");
}

/**
 * @brief Handles inbound WebSocket command messages.
 * @details Accepts text frames only and responds directly to originating
 * client with command result JSON.
 * @param clientNum WebSocket client id.
 * @param type Event type.
 * @param payload Raw payload bytes.
 * @param length Payload length.
 */
void onWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload,
                      size_t length) {
  markTransportActivity();
  if (gPortal == nullptr) return;
  if (type == WStype_CONNECTED) {
    if (kLogClientConnections) {
      const IPAddress remoteIp = gWsServer.remoteIP(clientNum);
      Serial.printf("[client] ws connected id=%u ip=%u.%u.%u.%u\n",
                    static_cast<unsigned>(clientNum), remoteIp[0], remoteIp[1],
                    remoteIp[2], remoteIp[3]);
      Serial.flush();
    }
    return;
  }
  if (type == WStype_DISCONNECTED) {
    if (kLogClientConnections) {
      Serial.printf("[client] ws disconnected id=%u\n",
                    static_cast<unsigned>(clientNum));
      Serial.flush();
    }
    return;
  }
  if (type != WStype_TEXT) return;

  String text;
  text.reserve(length + 1);
  for (size_t i = 0; i < length; ++i) {
    text += static_cast<char>(payload[i]);
  }

  const TransportCommandResponse response = handleTransportCommandStub(
      *gPortal, text.c_str(), TransportCommandSource::WebSocket, micros());
  String reply = response.body;
  gWsServer.sendTXT(clientNum, reply);
}

}  // namespace

/**
 * @brief Initializes portal HTTP/WebSocket transport routes and listeners.
 * @param portal Portal service instance backing request handling.
 * @par Used By
 * Main startup initialization.
 */
void initTransportRuntime(PortalService& portal,
                          v3::storage::StorageService& storage) {
  if (gTransportInitialized) return;
  gPortal = &portal;
  gStorage = &storage;
  gLastTransportActivityMs = millis();

  gHttpServer.on("/", HTTP_GET, handleHttpRootGet);
  gHttpServer.on("/index.html", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.index");
    if (!sendLittleFsFile("/index.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/index.html\"}");
    }
  });
  gHttpServer.on("/config", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.config");
    if (!sendLittleFsFile("/config.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/config\"}");
    }
  });
  gHttpServer.on("/config.html", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.config.html");
    if (!sendLittleFsFile("/config.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/config.html\"}");
    }
  });
  gHttpServer.on("/settings", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.settings");
    if (!sendLittleFsFile("/settings.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/settings\"}");
    }
  });
  gHttpServer.on("/settings.html", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.settings.html");
    if (!sendLittleFsFile("/settings.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/settings.html\"}");
    }
  });
  gHttpServer.on("/learn", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.learn");
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/learn\"}");
    }
  });
  gHttpServer.on("/learn.html", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.learn.html");
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/learn.html\"}");
    }
  });
  gHttpServer.on("/getting-started", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.getting-started");
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(
          404, "application/json",
          "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/getting-started\"}");
    }
  });
  gHttpServer.on("/tutorial", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.tutorial");
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(
          404, "application/json",
          "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/tutorial\"}");
    }
  });
  gHttpServer.on("/examples", HTTP_GET, []() {
    markTransportActivity();
    logHttpAccess("page.examples");
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(
          404, "application/json",
          "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/examples\"}");
    }
  });
  gHttpServer.on("/favicon.ico", HTTP_GET, sendNoContent);
  gHttpServer.on("/generate_204", HTTP_GET, sendNoContent);
  gHttpServer.on("/hotspot-detect.html", HTTP_GET, sendNoContent);
  gHttpServer.on("/ncsi.txt", HTTP_GET, sendNoContent);
  gHttpServer.on("/connecttest.txt", HTTP_GET, sendNoContent);

  gHttpServer.on("/api/v3/command", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/command/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/command", HTTP_POST, handleHttpCommandSubmit);
  gHttpServer.on("/api/v3/command/", HTTP_POST, handleHttpCommandSubmit);

  gHttpServer.on("/api/v3/diagnostics", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/diagnostics/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/diagnostics", HTTP_GET, handleHttpDiagnosticsGet);
  gHttpServer.on("/api/v3/diagnostics/", HTTP_GET, handleHttpDiagnosticsGet);

  gHttpServer.on("/api/v3/runtime/metrics", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/runtime/metrics/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/runtime/metrics", HTTP_GET, handleHttpRuntimeMetricsGet);
  gHttpServer.on("/api/v3/runtime/metrics/", HTTP_GET, handleHttpRuntimeMetricsGet);

  gHttpServer.on("/api/v3/runtime/cards", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/runtime/cards/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/runtime/cards", HTTP_GET, handleHttpRuntimeCardsGet);
  gHttpServer.on("/api/v3/runtime/cards/", HTTP_GET, handleHttpRuntimeCardsGet);
  gHttpServer.on("/api/v3/runtime/cards/delta", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/runtime/cards/delta/", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/runtime/cards/delta", HTTP_GET,
                 handleHttpRuntimeCardsDeltaGet);
  gHttpServer.on("/api/v3/runtime/cards/delta/", HTTP_GET,
                 handleHttpRuntimeCardsDeltaGet);

  gHttpServer.on("/api/v3/config/restore", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/restore/", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/restore", HTTP_POST,
                 handleHttpConfigRestorePost);
  gHttpServer.on("/api/v3/config/restore/", HTTP_POST,
                 handleHttpConfigRestorePost);

  gHttpServer.on("/api/v3/settings", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/settings/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/settings", HTTP_GET, handleHttpSettingsGet);
  gHttpServer.on("/api/v3/settings/", HTTP_GET, handleHttpSettingsGet);
  gHttpServer.on("/api/v3/settings", HTTP_PUT, handleHttpSettingsPut);
  gHttpServer.on("/api/v3/settings/", HTTP_PUT, handleHttpSettingsPut);

  gHttpServer.on("/api/v3/system/reboot", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/system/reboot/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/system/reboot", HTTP_POST, handleHttpSystemRebootPost);
  gHttpServer.on("/api/v3/system/reboot/", HTTP_POST, handleHttpSystemRebootPost);

  gHttpServer.on("/api/v3/cards", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/cards/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/cards", HTTP_GET, handleHttpCardsIndexGet);
  gHttpServer.on("/api/v3/cards/", HTTP_GET, handleHttpCardsIndexGet);
  gHttpServer.on(UriBraces("/api/v3/cards/{}"), HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on(UriBraces("/api/v3/cards/{}/"), HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on(UriBraces("/api/v3/cards/{}"), HTTP_GET,
                 handleHttpCardGetByPathArg);
  gHttpServer.on(UriBraces("/api/v3/cards/{}/"), HTTP_GET,
                 handleHttpCardGetByPathArg);
  gHttpServer.on(UriBraces("/api/v3/cards/{}"), HTTP_PUT,
                 handleHttpCardPutByPathArg);
  gHttpServer.on(UriBraces("/api/v3/cards/{}/"), HTTP_PUT,
                 handleHttpCardPutByPathArg);

  gHttpServer.onNotFound([]() {
    markTransportActivity();
    const HTTPMethod method = gHttpServer.method();
    const String uri = gHttpServer.uri();
    if (kLogTransport404) {
      Serial.printf("[transport] 404 method=%s path=%s\n",
                    methodToString(method), uri.c_str());
      const IPAddress remoteIp = gHttpServer.client().remoteIP();
      Serial.printf("[transport] 404 from %u.%u.%u.%u\n", remoteIp[0],
                    remoteIp[1], remoteIp[2], remoteIp[3]);
      Serial.flush();
    }

    String body = "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"";
    body += uri;
    body += "\",\"method\":\"";
    body += methodToString(method);
    body += "\"}";
    gHttpServer.send(404, "application/json", body);
  });
  gHttpServer.begin();

  gWsServer.begin();
  gWsServer.onEvent(onWebSocketEvent);

  gTransportInitialized = true;
}

/**
 * @brief Services HTTP and WebSocket event loops.
 * @par Used By
 * Main loop.
 */
void serviceTransportRuntime() {
  if (!gTransportInitialized) return;
  const uint32_t httpStartedMs = millis();
  feedTaskWatchdog();
  gHttpServer.handleClient();
  feedTaskWatchdog();
  logTransportTiming("http.handleClient", millis() - httpStartedMs);
  const uint32_t wsStartedMs = millis();
  gWsServer.loop();
  feedTaskWatchdog();
  logTransportTiming("ws.loop", millis() - wsStartedMs);
}

bool hasRecentTransportActivity(uint32_t nowMs, uint32_t windowMs) {
  if (!gTransportInitialized) return false;
  if (windowMs == 0) return true;
  const uint32_t elapsedMs = nowMs - gLastTransportActivityMs;
  return elapsedMs <= windowMs;
}

}  // namespace v3::portal
