/*
File: src/storage/storage_service.h
Purpose: Declares the storage service module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/main.cpp
- src/runtime/runtime_service.h
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include "storage/v3_config_validator.h"

namespace v3::storage {

enum class BootstrapSource : uint8_t { DefaultConfig, FileConfig };

struct BootstrapDiagnostics {
  BootstrapSource source;
  ConfigValidationError error;
  bool hasActiveConfig;
};

class StorageService {
 public:
  void begin();
  bool hasActiveConfig() const;
  const ValidatedConfig& activeConfig() const;
  const ConfigValidationError& lastError() const;
  BootstrapDiagnostics diagnostics() const;

 private:
  BootstrapSource source_ = BootstrapSource::DefaultConfig;
  ValidatedConfig activeConfig_ = {};
  ConfigValidationError lastError_ = {ConfigErrorCode::None, 0};
  bool activeConfigPresent_ = false;
};

}  // namespace v3::storage
