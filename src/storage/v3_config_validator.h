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

/**
 * @brief Enumerates config validation and decode error classes.
 * @details Provides stable machine-readable failure codes for bootstrap and API workflows.
 * @par Used By
 * - src/storage/v3_config_validator.cpp
 * - src/storage/storage_service.cpp
 * - src/runtime/runtime_service.cpp
 */
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

/**
 * @brief Identifies one validation failure with optional card index context.
 * @details `cardIndex` is meaningful for card-scoped failures and `0` otherwise.
 * @par Used By
 * - src/storage/v3_config_validator.cpp
 * - src/storage/storage_service.cpp
 */
struct ConfigValidationError {
  ConfigErrorCode code;
  uint8_t cardIndex;
};

/**
 * @brief Wraps a fully validated system configuration.
 * @details Serves as gate-proven contract consumed by runtime services.
 * @par Used By
 * - src/storage/storage_service.cpp
 * - src/kernel/kernel_service.h
 */
struct ValidatedConfig {
  SystemConfig system;
};

/**
 * @brief Result envelope for system config validation.
 * @details Contains either a validated config or the first detected validation error.
 * @par Used By
 * - src/storage/storage_service.cpp
 */
struct ConfigValidationResult {
  bool ok;
  ConfigValidationError error;
  ValidatedConfig validated;
};

/**
 * @brief Validates typed system config against V3 contract rules.
 * @details Enforces schema version, family/card constraints, and per-family parameter limits.
 * @param candidate Candidate system config to validate.
 * @return Validation result envelope.
 * @par Used By
 * - src/storage/storage_service.cpp
 */
ConfigValidationResult validateSystemConfig(const SystemConfig& candidate);
/**
 * @brief Validates typed system config against V3 contract rules (stack-light path).
 * @details Returns success/failure and fills `outError` without constructing large
 * result envelopes. Intended for low-stack boot paths.
 * @param candidate Candidate system config to validate.
 * @param outError Receives first validation error on failure; `None` on success.
 * @return `true` when config is valid.
 * @par Used By
 * - src/storage/storage_service.cpp
 */
bool validateSystemConfigLight(const SystemConfig& candidate,
                               ConfigValidationError& outError);
/**
 * @brief Converts config error code to stable display/debug text token.
 * @details Used for startup logs and diagnostics projection surfaces.
 * @param code Error code to stringify.
 * @return Static string token for the provided code.
 * @par Used By
 * - src/main.cpp
 * - src/storage/storage_service.cpp
 */
const char* configErrorCodeToString(ConfigErrorCode code);

}  // namespace v3::storage
