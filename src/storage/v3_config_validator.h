/*
File: src/storage/v3_config_validator.h
Purpose: Declares the v3 config validator module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/storage/storage_service.h
- src/storage/v3_config_decoder.h

Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include "storage/v3_config_contract.h"

namespace v3::storage {

enum class ConfigErrorCode : uint8_t {
  None,
  SchemaVersionMismatch,
  ScanIntervalOutOfRange,
  CardCountOutOfRange,
  DuplicateCardId,
  InvalidDiMode,
  InvalidDoMode,
  InvalidSioMode,
  InvalidDiDebounceStep,
  InvalidConditionBlock,
  InvalidAiRange,
  InvalidMathClamp,
  InvalidRtcTime,
  InvalidWiFiPolicy,
  ConfigPayloadInvalidJson,
  ConfigPayloadInvalidShape,
  ConfigPayloadUnknownFamily
};

struct ConfigValidationError {
  ConfigErrorCode code;
  uint8_t cardIndex;
};

struct ValidatedConfig {
  SystemConfig system;
};

struct ConfigValidationResult {
  bool ok;
  ConfigValidationError error;
  ValidatedConfig validated;
};

ConfigValidationResult validateSystemConfig(const SystemConfig& candidate);
const char* configErrorCodeToString(ConfigErrorCode code);

}  // namespace v3::storage
