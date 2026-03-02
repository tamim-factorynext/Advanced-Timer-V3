#pragma once

#include "storage/v3_config_contract.h"

namespace v3::storage {

enum class ConfigErrorCode : uint8_t {
  None,
  SchemaVersionMismatch,
  ScanIntervalOutOfRange,
  CardCountOutOfRange,
  DuplicateCardId,
  InvalidAiRange,
  InvalidMathClamp,
  InvalidRtcTime,
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
