/*
File: src/storage/storage_service.cpp
Purpose: Implements the storage service module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/main.cpp
- src/runtime/runtime_service.cpp
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "storage/storage_service.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <esp_task_wdt.h>
#include <stdio.h>

#include "storage/v3_config_decoder.h"

namespace v3::storage {

namespace {

constexpr const char* kLegacyActiveConfigPath = "/config_v3.json";
constexpr const char* kSplitRootDir = "/cfg";
constexpr const char* kSplitCardsDir = "/cfg/cards";
constexpr const char* kSplitSettingsPath = "/cfg/settings.json";
constexpr const char* kSplitCardsIndexPath = "/cfg/cards/index.json";

inline void feedBootWatchdog() {
  const esp_err_t err = esp_task_wdt_reset();
  (void)err;
}

bool ensureStorageFsMounted() {
  static bool mounted = false;
  if (mounted) return true;
  feedBootWatchdog();
  if (!LittleFS.begin()) {
    if (!LittleFS.begin(true)) return false;
  }
  mounted = true;
  return true;
}

const char* familyToString(const CardFamily family) {
  switch (family) {
    case CardFamily::DI:
      return "DI";
    case CardFamily::DO:
      return "DO";
    case CardFamily::AI:
      return "AI";
    case CardFamily::SIO:
      return "SIO";
    case CardFamily::MATH:
      return "MATH";
    case CardFamily::RTC:
      return "RTC";
    default:
      return "DI";
  }
}

const char* conditionOperatorToString(const ConditionOperator op) {
  switch (op) {
    case ConditionOperator::AlwaysTrue:
      return "ALWAYS_TRUE";
    case ConditionOperator::AlwaysFalse:
      return "ALWAYS_FALSE";
    case ConditionOperator::LogicalTrue:
      return "LOGICAL_TRUE";
    case ConditionOperator::LogicalFalse:
      return "LOGICAL_FALSE";
    case ConditionOperator::PhysicalOn:
      return "PHYSICAL_ON";
    case ConditionOperator::PhysicalOff:
      return "PHYSICAL_OFF";
    case ConditionOperator::Triggered:
      return "TRIGGERED";
    case ConditionOperator::TriggerCleared:
      return "TRIGGER_CLEARED";
    case ConditionOperator::GT:
      return "GT";
    case ConditionOperator::LT:
      return "LT";
    case ConditionOperator::EQ:
      return "EQ";
    case ConditionOperator::NEQ:
      return "NEQ";
    case ConditionOperator::GTE:
      return "GTE";
    case ConditionOperator::LTE:
      return "LTE";
    case ConditionOperator::Running:
      return "RUNNING";
    case ConditionOperator::Finished:
      return "FINISHED";
    case ConditionOperator::Stopped:
      return "STOPPED";
    default:
      return "ALWAYS_FALSE";
  }
}

const char* conditionCombinerToString(const ConditionCombiner combiner) {
  switch (combiner) {
    case ConditionCombiner::None:
      return "NONE";
    case ConditionCombiner::And:
      return "AND";
    case ConditionCombiner::Or:
      return "OR";
    default:
      return "NONE";
  }
}

void writeConditionClause(JsonObject dst, const ConditionClause& clause) {
  dst["sourceCardId"] = clause.sourceCardId;
  dst["operator"] = conditionOperatorToString(clause.op);
  dst["thresholdValue"] = clause.thresholdValue;
  dst["thresholdCardId"] = clause.thresholdCardId;
  dst["useThresholdCard"] = clause.useThresholdCard;
}

void writeConditionBlock(JsonObject dst, const ConditionBlock& block) {
  dst["combiner"] = conditionCombinerToString(block.combiner);
  JsonObject clauseA = dst["clauseA"].to<JsonObject>();
  writeConditionClause(clauseA, block.clauseA);
  JsonObject clauseB = dst["clauseB"].to<JsonObject>();
  writeConditionClause(clauseB, block.clauseB);
}

void writeSettingsJson(JsonObject out, const SystemConfig& cfg) {
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
  timeSync["secondaryServer"] = cfg.time.timeSync.secondaryServer;
  timeSync["tertiaryServer"] = cfg.time.timeSync.tertiaryServer;
  timeSync["syncIntervalSec"] = cfg.time.timeSync.syncIntervalSec;
  timeSync["startupTimeoutSec"] = cfg.time.timeSync.startupTimeoutSec;
  timeSync["maxTimeAgeSec"] = cfg.time.timeSync.maxTimeAgeSec;
}

void writeCardJson(JsonObject out, const CardConfig& card) {
  out["id"] = card.id;
  out["family"] = familyToString(card.family);
  out["enabled"] = card.enabled;
  JsonObject params = out["params"].to<JsonObject>();
  switch (card.family) {
    case CardFamily::DI:
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
    case CardFamily::DO:
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
    case CardFamily::AI:
      params["channel"] = card.ai.channel;
      params["inputMin"] = card.ai.inputMin;
      params["inputMax"] = card.ai.inputMax;
      params["outputMin"] = card.ai.outputMin;
      params["outputMax"] = card.ai.outputMax;
      params["smoothingFactorPct"] = card.ai.smoothingFactorPct;
      break;
    case CardFamily::SIO:
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
    case CardFamily::MATH:
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
    case CardFamily::RTC:
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

void buildCardPath(uint8_t cardId, char* outPath, size_t outPathSize) {
  snprintf(outPath, outPathSize, "/cfg/cards/%u.json",
           static_cast<unsigned>(cardId));
}

bool writeJsonAtomic(const char* path, JsonDocument& doc) {
  char tmpPath[96] = {};
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);

  File file = LittleFS.open(tmpPath, "w");
  if (!file) return false;
  const size_t written = serializeJson(doc, file);
  file.flush();
  file.close();
  if (written == 0) {
    LittleFS.remove(tmpPath);
    return false;
  }
  LittleFS.remove(path);
  if (!LittleFS.rename(tmpPath, path)) {
    LittleFS.remove(tmpPath);
    return false;
  }
  return true;
}

bool ensureSplitDirs() {
  if (!LittleFS.exists(kSplitRootDir)) {
    if (!LittleFS.mkdir(kSplitRootDir)) return false;
  }
  if (!LittleFS.exists(kSplitCardsDir)) {
    if (!LittleFS.mkdir(kSplitCardsDir)) return false;
  }
  return true;
}

bool persistSplitConfig(const SystemConfig& cfg) {
  if (!ensureStorageFsMounted()) return false;
  if (!ensureSplitDirs()) return false;

  for (uint8_t i = 0; i < cfg.cardCount; ++i) {
    JsonDocument cardDoc;
    JsonObject cardRoot = cardDoc.to<JsonObject>();
    writeCardJson(cardRoot, cfg.cards[i]);
    char cardPath[64] = {};
    buildCardPath(cfg.cards[i].id, cardPath, sizeof(cardPath));
    if (!writeJsonAtomic(cardPath, cardDoc)) return false;
  }

  JsonDocument settingsDoc;
  JsonObject settingsRoot = settingsDoc.to<JsonObject>();
  writeSettingsJson(settingsRoot, cfg);
  if (!writeJsonAtomic(kSplitSettingsPath, settingsDoc)) return false;

  JsonDocument indexDoc;
  JsonObject indexRoot = indexDoc.to<JsonObject>();
  indexRoot["layoutVersion"] = 1;
  indexRoot["cardCount"] = cfg.cardCount;
  JsonArray cards = indexRoot["cards"].to<JsonArray>();
  for (uint8_t i = 0; i < cfg.cardCount; ++i) {
    JsonObject item = cards.add<JsonObject>();
    item["id"] = cfg.cards[i].id;
    item["family"] = familyToString(cfg.cards[i].family);
    item["enabled"] = cfg.cards[i].enabled;
  }
  if (!writeJsonAtomic(kSplitCardsIndexPath, indexDoc)) return false;

  LittleFS.remove(kLegacyActiveConfigPath);
  return true;
}

bool tryLoadCandidateFromSplit(SystemConfig& outCandidate,
                               ConfigValidationError& outError) {
  outError = {ConfigErrorCode::None, 0};
  if (!ensureStorageFsMounted()) return false;
  if (!LittleFS.exists(kSplitSettingsPath) || !LittleFS.exists(kSplitCardsIndexPath)) {
    return false;
  }

  File settingsFile = LittleFS.open(kSplitSettingsPath, "r");
  if (!settingsFile) return false;
  JsonDocument settingsDoc;
  const DeserializationError settingsJsonErr =
      deserializeJson(settingsDoc, settingsFile);
  settingsFile.close();
  if (settingsJsonErr) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidJson, 0};
    return true;
  }
  if (!decodeSystemSettingsLight(settingsDoc.as<JsonObjectConst>(), outCandidate,
                                 outError)) {
    return true;
  }

  File indexFile = LittleFS.open(kSplitCardsIndexPath, "r");
  if (!indexFile) return false;
  JsonDocument indexDoc;
  const DeserializationError indexJsonErr = deserializeJson(indexDoc, indexFile);
  indexFile.close();
  if (indexJsonErr) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidJson, 0};
    return true;
  }
  JsonArrayConst indexCards = indexDoc["cards"].as<JsonArrayConst>();
  if (indexCards.isNull() || indexCards.size() > kMaxCards) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidShape, 0};
    return true;
  }

  outCandidate.cardCount = 0;
  uint8_t cardIndex = 0;
  for (JsonVariantConst entry : indexCards) {
    uint8_t cardId = 0;
    if (entry.is<JsonObjectConst>()) {
      JsonObjectConst item = entry.as<JsonObjectConst>();
      if (!item["id"].is<uint8_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return true;
      }
      cardId = item["id"].as<uint8_t>();
    } else if (entry.is<uint8_t>()) {
      cardId = entry.as<uint8_t>();
    } else {
      outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
      return true;
    }

    char cardPath[64] = {};
    buildCardPath(cardId, cardPath, sizeof(cardPath));
    File cardFile = LittleFS.open(cardPath, "r");
    if (!cardFile) {
      outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
      return true;
    }
    JsonDocument cardDoc;
    const DeserializationError cardJsonErr = deserializeJson(cardDoc, cardFile);
    cardFile.close();
    if (cardJsonErr) {
      outError = {ConfigErrorCode::ConfigPayloadInvalidJson, cardIndex};
      return true;
    }
    CardConfig card = {};
    if (!decodeCardConfigLight(cardDoc.as<JsonObjectConst>(), card, outError,
                               cardIndex)) {
      return true;
    }
    if (card.id != cardId) {
      outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
      return true;
    }
    outCandidate.cards[cardIndex] = card;
    cardIndex += 1;
    outCandidate.cardCount = cardIndex;
  }

  return true;
}

/**
 * @brief Attempts to load and decode candidate config from persistent file.
 * @details Returns `false` when file path is absent/unreadable. Returns `true`
 * when a load attempt occurred, even if decode failed (error is populated).
 * @param outCandidate Decoded candidate config on success.
 * @param outError Decode error details when payload is invalid.
 * @retval true Config file existed and was processed.
 * @retval false Config file unavailable or unreadable.
 * @par Used By
 * StorageService::begin().
 */
bool tryLoadCandidateFromFile(SystemConfig& outCandidate,
                              ConfigValidationError& outError) {
  feedBootWatchdog();
  Serial.println("[storage:file] 01 LittleFS.begin()");
  Serial.printf("[storage:file] heap before begin=%lu\n",
                static_cast<unsigned long>(ESP.getFreeHeap()));
  Serial.flush();
  feedBootWatchdog();
  if (!LittleFS.begin()) {
    Serial.println("[storage:file] 02 LittleFS.begin failed");
    Serial.println("[storage:file] 02b retry LittleFS.begin(true) to format");
    Serial.flush();
    feedBootWatchdog();
    if (!LittleFS.begin(true)) {
      Serial.println("[storage:file] 02c LittleFS.begin(true) failed");
      Serial.flush();
      feedBootWatchdog();
      return false;
    }
    Serial.println("[storage:file] 02d LittleFS formatted + mounted");
    Serial.flush();
    feedBootWatchdog();
  }
  Serial.println("[storage:file] 03 LittleFS.begin ok");
  Serial.flush();
  feedBootWatchdog();
  Serial.println("[storage:file] 04 open /config_v3.json (read)");
  Serial.flush();
  feedBootWatchdog();
  File file = LittleFS.open(kLegacyActiveConfigPath, "r");
  if (!file) {
    Serial.println("[storage:file] 05 config file missing");
    Serial.flush();
    feedBootWatchdog();
    return false;
  }
  Serial.println("[storage:file] 06 open ok, deserialize");
  Serial.flush();
  feedBootWatchdog();

  JsonDocument doc;
  const DeserializationError jsonError = deserializeJson(doc, file);
  feedBootWatchdog();
  file.close();
  if (jsonError) {
    Serial.println("[storage:file] 07 json invalid");
    Serial.flush();
    feedBootWatchdog();
    outError = {ConfigErrorCode::ConfigPayloadInvalidJson, 0};
    return true;
  }

  Serial.println("[storage:file] 08 decodeSystemConfig");
  Serial.flush();
  feedBootWatchdog();
  ConfigValidationError decodeError = {ConfigErrorCode::None, 0};
  if (!decodeSystemConfigLight(doc.as<JsonObjectConst>(), outCandidate,
                               decodeError)) {
    Serial.println("[storage:file] 09 decode failed");
    Serial.flush();
    feedBootWatchdog();
    outError = decodeError;
    return true;
  }

  Serial.println("[storage:file] 10 decode ok");
  Serial.flush();
  feedBootWatchdog();
  return true;
}

}  // namespace

StorageService::~StorageService() {
  if (stagedConfig_ != nullptr) {
    delete stagedConfig_;
    stagedConfig_ = nullptr;
  }
}

bool StorageService::ensureConfigBuffer(SystemConfig*& target) {
  if (target != nullptr) return true;
  target = new SystemConfig();
  return target != nullptr;
}

/**
 * @brief Bootstraps active config from file or default template, then validates.
 * @details Establishes storage diagnostics state for runtime telemetry and
 * decides whether an active config is present.
 * @par Used By
 * Main startup initialization.
 */
void StorageService::begin() {
  auto logStorageStage = [](const char* stage) {
    Serial.print("[storage] ");
    Serial.println(stage);
    Serial.flush();
    feedBootWatchdog();
  };

  logStorageStage("01 begin");
  logStorageStage("02 build default config");
  SystemConfig& candidate = activeConfig_.system;
  makeDefaultSystemConfig(candidate);
  logStorageStage("03 default config done");
  ConfigValidationError bootstrapError = {ConfigErrorCode::None, 0};

  logStorageStage("04 try load split config");
  const bool attemptedSplitLoad =
      tryLoadCandidateFromSplit(candidate, bootstrapError);
  logStorageStage("05 split load returned");

  bool attemptedLegacyLoad = false;
  if (!attemptedSplitLoad) {
    logStorageStage("06 try load legacy config");
    attemptedLegacyLoad = tryLoadCandidateFromFile(candidate, bootstrapError);
    logStorageStage("07 legacy load returned");
  }

  const bool attemptedFileLoad = attemptedSplitLoad || attemptedLegacyLoad;
  source_ =
      attemptedFileLoad ? BootstrapSource::FileConfig : BootstrapSource::DefaultConfig;

  logStorageStage("08 check bootstrap error");
  if (attemptedFileLoad && bootstrapError.code != ConfigErrorCode::None) {
    activeConfigPresent_ = false;
    lastError_ = bootstrapError;
    logStorageStage("09 bootstrap error path exit");
    return;
  }

  logStorageStage("10 validate candidate");
  ConfigValidationError validationError = {ConfigErrorCode::None, 0};
  const bool valid = validateSystemConfigLight(candidate, validationError);
  logStorageStage("11 validate returned");

  if (!valid) {
    activeConfigPresent_ = false;
    lastError_ = validationError;
    logStorageStage("12 validation failed path exit");
    return;
  }

  if (!attemptedSplitLoad && attemptedLegacyLoad) {
    logStorageStage("13 migrate legacy -> split");
    if (!persistSplitConfig(candidate)) {
      Serial.println("[storage] legacy migration write failed; continuing in-RAM");
      Serial.flush();
    }
  }

  lastError_ = {ConfigErrorCode::None, 0};
  activeConfigPresent_ = true;
  if (activeRevision_ == 0) {
    activeRevision_ = 1;
  } else {
    activeRevision_ += 1;
  }
  logStorageStage("14 success exit");
}

/**
 * @brief Indicates whether validated active config is available.
 * @return `true` when active config exists.
 */
bool StorageService::hasActiveConfig() const { return activeConfigPresent_; }

/**
 * @brief Returns validated active config payload.
 * @return Active config reference.
 */
const ValidatedConfig& StorageService::activeConfig() const {
  return activeConfig_;
}

/**
 * @brief Returns last bootstrap/decode/validation error.
 * @return Error structure.
 */
const ConfigValidationError& StorageService::lastError() const {
  return lastError_;
}

/**
 * @brief Builds storage bootstrap diagnostics snapshot.
 * @return Current diagnostics values.
 * @par Used By
 * RuntimeService::tick().
 */
BootstrapDiagnostics StorageService::diagnostics() const {
  BootstrapDiagnostics diagnostics = {};
  diagnostics.source = source_;
  diagnostics.error = lastError_;
  diagnostics.hasActiveConfig = activeConfigPresent_;
  return diagnostics;
}

const SystemConfig& StorageService::activeSystemConfig() const {
  return activeConfig_.system;
}

const SystemConfig& StorageService::stagedSystemConfig() const {
  if (stagedConfig_ != nullptr) return *stagedConfig_;
  return activeConfig_.system;
}

bool StorageService::hasStagedConfig() const { return stagedConfigPresent_; }

void StorageService::stageConfig(const ValidatedConfig& validated) {
  if (!ensureConfigBuffer(stagedConfig_)) return;
  *stagedConfig_ = validated.system;
  stagedConfigPresent_ = true;
  stagedRevision_ += 1;
}

bool StorageService::commitStaged() {
  if (!stagedConfigPresent_ || stagedConfig_ == nullptr) return false;
  if (!persistSplitConfig(*stagedConfig_)) return false;
  activeConfig_.system = *stagedConfig_;
  activeConfigPresent_ = true;
  stagedConfigPresent_ = false;
  activeRevision_ += 1;
  return true;
}

bool StorageService::restoreFactory() {
  ValidatedConfig factoryValidated = {};
  makeDefaultSystemConfig(factoryValidated.system);
  ConfigValidationError validationError = {ConfigErrorCode::None, 0};
  if (!validateSystemConfigLight(factoryValidated.system, validationError)) {
    lastError_ = validationError;
    return false;
  }
  if (!persistSplitConfig(factoryValidated.system)) return false;
  activeConfig_ = factoryValidated;
  activeConfigPresent_ = true;
  activeRevision_ += 1;
  lastError_ = {ConfigErrorCode::None, 0};
  stagedConfigPresent_ = false;
  return true;
}

uint32_t StorageService::activeRevision() const { return activeRevision_; }

uint32_t StorageService::stagedRevision() const { return stagedRevision_; }

}  // namespace v3::storage
