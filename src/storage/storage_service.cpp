#include "storage/storage_service.h"

namespace v3::storage {

void StorageService::begin() {
  const SystemConfig defaultCandidate = makeDefaultSystemConfig();
  const ConfigValidationResult validation =
      validateSystemConfig(defaultCandidate);

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

}  // namespace v3::storage
