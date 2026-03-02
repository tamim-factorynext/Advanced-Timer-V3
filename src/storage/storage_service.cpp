#include "storage/storage_service.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "storage/v3_config_decoder.h"

namespace v3::storage {

namespace {

constexpr const char* kActiveConfigPath = "/config_v3.json";

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

bool StorageService::hasActiveConfig() const { return activeConfigPresent_; }

const ValidatedConfig& StorageService::activeConfig() const {
  return activeConfig_;
}

const ConfigValidationError& StorageService::lastError() const {
  return lastError_;
}

BootstrapDiagnostics StorageService::diagnostics() const {
  BootstrapDiagnostics diagnostics = {};
  diagnostics.source = source_;
  diagnostics.error = lastError_;
  diagnostics.hasActiveConfig = activeConfigPresent_;
  return diagnostics;
}

}  // namespace v3::storage
