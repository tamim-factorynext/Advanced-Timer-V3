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

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "storage/v3_config_decoder.h"

namespace v3::storage {

namespace {

constexpr const char* kActiveConfigPath = "/config_v3.json";

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
  if (!LittleFS.begin()) return false;
  if (!LittleFS.exists(kActiveConfigPath)) return false;

  File file = LittleFS.open(kActiveConfigPath, "r");
  if (!file) return false;

  JsonDocument doc;
  const DeserializationError jsonError = deserializeJson(doc, file);
  file.close();
  if (jsonError) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidJson, 0};
    return true;
  }

  const ConfigDecodeResult decoded = decodeSystemConfig(doc.as<JsonObjectConst>());
  if (!decoded.ok) {
    outError = decoded.error;
    return true;
  }

  outCandidate = decoded.decoded;
  return true;
}

}  // namespace

/**
 * @brief Bootstraps active config from file or default template, then validates.
 * @details Establishes storage diagnostics state for runtime telemetry and
 * decides whether an active config is present.
 * @par Used By
 * Main startup initialization.
 */
void StorageService::begin() {
  SystemConfig candidate = makeDefaultSystemConfig();
  ConfigValidationError bootstrapError = {ConfigErrorCode::None, 0};

  const bool attemptedFileLoad =
      tryLoadCandidateFromFile(candidate, bootstrapError);
  source_ = attemptedFileLoad ? BootstrapSource::FileConfig
                              : BootstrapSource::DefaultConfig;

  if (attemptedFileLoad && bootstrapError.code != ConfigErrorCode::None) {
    activeConfigPresent_ = false;
    lastError_ = bootstrapError;
    return;
  }

  const ConfigValidationResult validation = validateSystemConfig(candidate);

  if (!validation.ok) {
    activeConfigPresent_ = false;
    lastError_ = validation.error;
    return;
  }

  activeConfig_ = validation.validated;
  lastError_ = {ConfigErrorCode::None, 0};
  activeConfigPresent_ = true;
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

}  // namespace v3::storage
