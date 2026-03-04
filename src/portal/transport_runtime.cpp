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

#include <cstring>

#include <LittleFS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

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

void sendNoContent() { gHttpServer.send(204, "text/plain", ""); }
void markTransportActivity() { gLastTransportActivityMs = millis(); }

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
  if (!LittleFS.exists(path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  gHttpServer.streamFile(file, contentTypeForPath(String(path)));
  file.close();
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
  if (block.combiner != v3::storage::ConditionCombiner::None) {
    JsonObject clauseB = dst["clauseB"].to<JsonObject>();
    writeConditionClause(clauseB, block.clauseB);
  }
}

void writeSystemConfigJson(JsonObject out,
                           const v3::storage::SystemConfig& cfg) {
  out["schemaVersion"] = cfg.schemaVersion;
  out["scanPeriodMs"] = cfg.scanPeriodMs;

  JsonObject system = out["system"].to<JsonObject>();
  JsonObject wifi = system["wifi"].to<JsonObject>();

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

  JsonObject time = system["time"].to<JsonObject>();
  time["timezone"] = cfg.time.timezone;
  JsonObject timeSync = time["timeSync"].to<JsonObject>();
  timeSync["enabled"] = cfg.time.timeSync.enabled;
  timeSync["primaryTimeServer"] = cfg.time.timeSync.primaryTimeServer;
  timeSync["secondaryServer"] = cfg.time.timeSync.secondaryServer;
  timeSync["tertiaryServer"] = cfg.time.timeSync.tertiaryServer;
  timeSync["syncIntervalSec"] = cfg.time.timeSync.syncIntervalSec;
  timeSync["startupTimeoutSec"] = cfg.time.timeSync.startupTimeoutSec;
  timeSync["maxTimeAgeSec"] = cfg.time.timeSync.maxTimeAgeSec;

  JsonArray cards = out["cards"].to<JsonArray>();
  for (uint8_t i = 0; i < cfg.cardCount; ++i) {
    const v3::storage::CardConfig& card = cfg.cards[i];
    JsonObject item = cards.add<JsonObject>();
    item["id"] = card.id;
    item["family"] = familyToString(card.family);
    item["enabled"] = card.enabled;
    JsonObject params = item["params"].to<JsonObject>();
    switch (card.family) {
      case v3::storage::CardFamily::DI:
        params["channel"] = card.di.channel;
        params["invert"] = card.di.invert;
        params["debounceMs"] = card.di.debounceMs;
        params["edgeMode"] = card.di.edgeMode;
        params["setEnabled"] = card.di.setEnabled;
        params["resetEnabled"] = card.di.resetEnabled;
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
        params["inputA"] = card.math.inputA;
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
}

bool extractConfigObject(JsonDocument& requestDoc, JsonObjectConst& outConfigObj,
                         String& outRequestId) {
  if (!requestDoc.is<JsonObjectConst>()) return false;
  const JsonObjectConst root = requestDoc.as<JsonObjectConst>();
  outRequestId = root["requestId"].is<const char*>()
                     ? String(root["requestId"].as<const char*>())
                     : String();
  if (root["config"].is<JsonObjectConst>()) {
    outConfigObj = root["config"].as<JsonObjectConst>();
    return true;
  }
  outConfigObj = root;
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

/**
 * @brief Handles HTTP runtime snapshot endpoint.
 * @par Used By
 * `GET /api/v3/snapshot`.
 */
void handleHttpSnapshotGet() {
  markTransportActivity();
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }
  const PortalSnapshotState state = gPortal->snapshotState();
  if (!state.ready || state.json == nullptr || state.json[0] == '\0') {
    gHttpServer.send(503, "application/json",
                     "{\"ok\":false,\"reason\":\"snapshot_not_ready\"}");
    return;
  }
  gHttpServer.send(200, "application/json", state.json);
}

/**
 * @brief Handles HTTP diagnostics endpoint.
 * @par Used By
 * `GET /api/v3/diagnostics`.
 */
void handleHttpDiagnosticsGet() {
  markTransportActivity();
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

void handleHttpConfigActiveGet() {
  markTransportActivity();
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
  doc["schemaVersion"] = gStorage->activeSystemConfig().schemaVersion;
  JsonObject cfg = doc["config"].to<JsonObject>();
  writeSystemConfigJson(cfg, gStorage->activeSystemConfig());
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpConfigStagedSavePost() {
  markTransportActivity();
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
  JsonObjectConst cfgObj;
  String requestId;
  if (!extractConfigObject(requestDoc, cfgObj, requestId)) {
    sendConfigError(400, "INVALID_REQUEST", requestId, "invalid config wrapper");
    return;
  }
  v3::storage::SystemConfig candidate = {};
  v3::storage::ConfigValidationError decodeError = {
      v3::storage::ConfigErrorCode::None, 0};
  if (!v3::storage::decodeSystemConfigLight(cfgObj, candidate, decodeError)) {
    sendConfigError(422, "VALIDATION_FAILED", requestId,
                    v3::storage::configErrorCodeToString(decodeError.code));
    return;
  }
  v3::storage::ConfigValidationError validationError = {
      v3::storage::ConfigErrorCode::None, 0};
  if (!v3::storage::validateSystemConfigLight(candidate, validationError)) {
    sendConfigError(422, "VALIDATION_FAILED", requestId,
                    v3::storage::configErrorCodeToString(validationError.code));
    return;
  }
  v3::storage::ValidatedConfig staged = {};
  staged.system = candidate;
  gStorage->stageConfig(staged);
  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["requestId"] = requestId;
  doc["tsMs"] = millis();
  doc["stagedVersion"] = gStorage->stagedRevision();
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpConfigStagedValidatePost() {
  markTransportActivity();
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

  JsonObjectConst cfgObj;
  String requestId;
  if (!extractConfigObject(requestDoc, cfgObj, requestId)) {
    sendConfigError(400, "INVALID_REQUEST", requestId, "invalid config wrapper");
    return;
  }

  v3::storage::SystemConfig candidate = {};
  v3::storage::ConfigValidationError decodeError = {
      v3::storage::ConfigErrorCode::None, 0};
  if (!v3::storage::decodeSystemConfigLight(cfgObj, candidate, decodeError)) {
    sendConfigError(422, "VALIDATION_FAILED", requestId,
                    v3::storage::configErrorCodeToString(decodeError.code));
    return;
  }

  v3::storage::ConfigValidationError validationError = {
      v3::storage::ConfigErrorCode::None, 0};
  if (!v3::storage::validateSystemConfigLight(candidate, validationError)) {
    sendConfigError(422, "VALIDATION_FAILED", requestId,
                    v3::storage::configErrorCodeToString(validationError.code));
    return;
  }

  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["requestId"] = requestId;
  doc["tsMs"] = millis();
  JsonObject validation = doc["validation"].to<JsonObject>();
  validation["errors"].to<JsonArray>();
  validation["warnings"].to<JsonArray>();
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpConfigCommitPost() {
  markTransportActivity();
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
  JsonObjectConst cfgObj;
  String requestId;
  if (!extractConfigObject(requestDoc, cfgObj, requestId)) {
    sendConfigError(400, "INVALID_REQUEST", requestId, "invalid config wrapper");
    return;
  }
  if (cfgObj.size() > 0 && requestDoc["config"].is<JsonObjectConst>()) {
    v3::storage::SystemConfig candidate = {};
    v3::storage::ConfigValidationError decodeError = {
        v3::storage::ConfigErrorCode::None, 0};
    if (!v3::storage::decodeSystemConfigLight(cfgObj, candidate, decodeError)) {
      sendConfigError(422, "VALIDATION_FAILED", requestId,
                      v3::storage::configErrorCodeToString(decodeError.code));
      return;
    }
    v3::storage::ConfigValidationError validationError = {
        v3::storage::ConfigErrorCode::None, 0};
    if (!v3::storage::validateSystemConfigLight(candidate, validationError)) {
      sendConfigError(422, "VALIDATION_FAILED", requestId,
                      v3::storage::configErrorCodeToString(validationError.code));
      return;
    }
    v3::storage::ValidatedConfig staged = {};
    staged.system = candidate;
    gStorage->stageConfig(staged);
  }
  if (!gStorage->commitStaged()) {
    sendConfigError(409, "COMMIT_FAILED", requestId, "no staged config to commit");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["apiVersion"] = "2.0";
  doc["status"] = "SUCCESS";
  doc["requestId"] = requestId;
  doc["tsMs"] = millis();
  doc["activeVersion"] = gStorage->activeRevision();
  JsonObject historyHead = doc["historyHead"].to<JsonObject>();
  historyHead["lkgVersion"] = gStorage->lkgRevision();
  doc["requiresRestart"] = true;
  String body;
  serializeJson(doc, body);
  gHttpServer.send(200, "application/json", body);
}

void handleHttpConfigRestorePost() {
  markTransportActivity();
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
  bool restored = false;
  if (strcmp(source, "FACTORY") == 0) {
    restored = gStorage->restoreFactory();
  } else if (strcmp(source, "LKG") == 0) {
    restored = gStorage->restoreLkg();
  } else {
    sendConfigError(422, "RESTORE_FAILED", requestId, "unsupported source");
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

/**
 * @brief Handles root landing endpoint.
 * @details Returns a tiny discovery payload with primary API routes so raw-IP
 * browser opens get a valid response.
 * @par Used By
 * `GET /`.
 */
void handleHttpRootGet() {
  markTransportActivity();
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
      "snapshot\",\"/api/v3/diagnostics\",\"/api/v3/command\","
      "\"/api/v3/config/active\",\"/api/v3/config/staged/save\","
      "\"/api/v3/config/staged/validate\",\"/api/v3/config/commit\","
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
    if (!sendLittleFsFile("/index.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/index.html\"}");
    }
  });
  gHttpServer.on("/config", HTTP_GET, []() {
    markTransportActivity();
    if (!sendLittleFsFile("/config.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/config\"}");
    }
  });
  gHttpServer.on("/config.html", HTTP_GET, []() {
    markTransportActivity();
    if (!sendLittleFsFile("/config.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/config.html\"}");
    }
  });
  gHttpServer.on("/settings", HTTP_GET, []() {
    markTransportActivity();
    if (!sendLittleFsFile("/settings.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/settings\"}");
    }
  });
  gHttpServer.on("/settings.html", HTTP_GET, []() {
    markTransportActivity();
    if (!sendLittleFsFile("/settings.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/settings.html\"}");
    }
  });
  gHttpServer.on("/learn", HTTP_GET, []() {
    markTransportActivity();
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/learn\"}");
    }
  });
  gHttpServer.on("/learn.html", HTTP_GET, []() {
    markTransportActivity();
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(404, "application/json",
                       "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/learn.html\"}");
    }
  });
  gHttpServer.on("/getting-started", HTTP_GET, []() {
    markTransportActivity();
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(
          404, "application/json",
          "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/getting-started\"}");
    }
  });
  gHttpServer.on("/tutorial", HTTP_GET, []() {
    markTransportActivity();
    if (!sendLittleFsFile("/learn.html")) {
      gHttpServer.send(
          404, "application/json",
          "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"/tutorial\"}");
    }
  });
  gHttpServer.on("/examples", HTTP_GET, []() {
    markTransportActivity();
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

  gHttpServer.on("/api/v3/snapshot", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/snapshot/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/snapshot", HTTP_GET, handleHttpSnapshotGet);
  gHttpServer.on("/api/v3/snapshot/", HTTP_GET, handleHttpSnapshotGet);
  gHttpServer.on("/api/v3/config/active", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/active/", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/active", HTTP_GET, handleHttpConfigActiveGet);
  gHttpServer.on("/api/v3/config/active/", HTTP_GET, handleHttpConfigActiveGet);

  gHttpServer.on("/api/v3/config/staged/save", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/staged/save/", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/staged/save", HTTP_POST,
                 handleHttpConfigStagedSavePost);
  gHttpServer.on("/api/v3/config/staged/save/", HTTP_POST,
                 handleHttpConfigStagedSavePost);

  gHttpServer.on("/api/v3/config/staged/validate", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/staged/validate/", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/staged/validate", HTTP_POST,
                 handleHttpConfigStagedValidatePost);
  gHttpServer.on("/api/v3/config/staged/validate/", HTTP_POST,
                 handleHttpConfigStagedValidatePost);

  gHttpServer.on("/api/v3/config/commit", HTTP_OPTIONS, handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/commit/", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/commit", HTTP_POST, handleHttpConfigCommitPost);
  gHttpServer.on("/api/v3/config/commit/", HTTP_POST,
                 handleHttpConfigCommitPost);

  gHttpServer.on("/api/v3/config/restore", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/restore/", HTTP_OPTIONS,
                 handleHttpCorsOptions);
  gHttpServer.on("/api/v3/config/restore", HTTP_POST,
                 handleHttpConfigRestorePost);
  gHttpServer.on("/api/v3/config/restore/", HTTP_POST,
                 handleHttpConfigRestorePost);
  gHttpServer.onNotFound([]() {
    markTransportActivity();
    const HTTPMethod method = gHttpServer.method();
    Serial.printf("[transport] 404 method=%s path=%s\n", methodToString(method),
                  gHttpServer.uri().c_str());
    Serial.flush();

    String body = "{\"ok\":false,\"reason\":\"not_found\",\"path\":\"";
    body += gHttpServer.uri();
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
  gHttpServer.handleClient();
  gWsServer.loop();
}

bool hasRecentTransportActivity(uint32_t nowMs, uint32_t windowMs) {
  if (!gTransportInitialized) return false;
  if (windowMs == 0) return true;
  const uint32_t elapsedMs = nowMs - gLastTransportActivityMs;
  return elapsedMs <= windowMs;
}

}  // namespace v3::portal
