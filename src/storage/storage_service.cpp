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

#include "storage/v3_config_decoder.h"

namespace v3::storage {

namespace {

constexpr const char* kActiveConfigPath = "/config_v3.json";

inline void feedBootWatchdog() {
  const esp_err_t err = esp_task_wdt_reset();
  (void)err;
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
  File file = LittleFS.open(kActiveConfigPath, "r");
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
  if (lkgConfig_ != nullptr) {
    delete lkgConfig_;
    lkgConfig_ = nullptr;
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

  logStorageStage("04 try load from file");
  const bool attemptedFileLoad =
      tryLoadCandidateFromFile(candidate, bootstrapError);
  logStorageStage("05 file load returned");
  source_ = attemptedFileLoad ? BootstrapSource::FileConfig
                              : BootstrapSource::DefaultConfig;

  logStorageStage("06 check bootstrap error");
  if (attemptedFileLoad && bootstrapError.code != ConfigErrorCode::None) {
    activeConfigPresent_ = false;
    lastError_ = bootstrapError;
    logStorageStage("07 bootstrap error path exit");
    return;
  }

  logStorageStage("08 validate candidate");
  ConfigValidationError validationError = {ConfigErrorCode::None, 0};
  const bool valid = validateSystemConfigLight(candidate, validationError);
  logStorageStage("09 validate returned");

  if (!valid) {
    activeConfigPresent_ = false;
    lastError_ = validationError;
    logStorageStage("10 validation failed path exit");
    return;
  }

  lastError_ = {ConfigErrorCode::None, 0};
  activeConfigPresent_ = true;
  if (!lkgConfigPresent_ && ensureConfigBuffer(lkgConfig_)) {
    *lkgConfig_ = activeConfig_.system;
    lkgConfigPresent_ = true;
    lkgRevision_ = 1;
  }
  if (activeRevision_ == 0) {
    activeRevision_ = 1;
  } else {
    activeRevision_ += 1;
  }
  logStorageStage("11 success exit");
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
  if (activeConfigPresent_) {
    if (!ensureConfigBuffer(lkgConfig_)) return false;
    *lkgConfig_ = activeConfig_.system;
    lkgConfigPresent_ = true;
    lkgRevision_ += 1;
  }
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
  if (activeConfigPresent_) {
    if (!ensureConfigBuffer(lkgConfig_)) return false;
    *lkgConfig_ = activeConfig_.system;
    lkgConfigPresent_ = true;
    lkgRevision_ += 1;
  }
  activeConfig_ = factoryValidated;
  activeConfigPresent_ = true;
  activeRevision_ += 1;
  lastError_ = {ConfigErrorCode::None, 0};
  stagedConfigPresent_ = false;
  return true;
}

bool StorageService::restoreLkg() {
  if (!lkgConfigPresent_ || lkgConfig_ == nullptr) return false;
  if (activeConfigPresent_) {
    if (ensureConfigBuffer(stagedConfig_)) {
      *stagedConfig_ = activeConfig_.system;
      stagedConfigPresent_ = true;
      stagedRevision_ += 1;
    }
  }
  activeConfig_.system = *lkgConfig_;
  activeConfigPresent_ = true;
  activeRevision_ += 1;
  lastError_ = {ConfigErrorCode::None, 0};
  return true;
}

uint32_t StorageService::activeRevision() const { return activeRevision_; }

uint32_t StorageService::stagedRevision() const { return stagedRevision_; }

uint32_t StorageService::lkgRevision() const { return lkgRevision_; }

}  // namespace v3::storage
