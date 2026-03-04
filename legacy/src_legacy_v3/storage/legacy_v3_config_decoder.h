/*
File: src/storage/legacy_v3_config_decoder.h
Purpose: Declares the v3 config decoder module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/storage/storage_service.cpp
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <ArduinoJson.h>

#include "storage/legacy_v3_config_contract.h"
#include "storage/legacy_v3_config_validator.h"

namespace v3::storage {

/**
 * @brief Result envelope for config payload decode operation.
 * @details Carries success flag, decode/shape error, and decoded system contract.
 * @par Used By
 * - src/storage/storage_service.cpp
 */
struct ConfigDecodeResult {
  bool ok;
  ConfigValidationError error;
  SystemConfig decoded;
};

/**
 * @brief Decodes JSON config payload into typed `SystemConfig`.
 * @details Enforces expected payload shape and field typing before validator stage.
 * @param root Root JSON object of the candidate config payload.
 * @return Decode result envelope with either typed config or error code.
 * @par Used By
 * - src/storage/storage_service.cpp
 */
ConfigDecodeResult decodeSystemConfig(JsonObjectConst root);
/**
 * @brief Decodes JSON config payload into typed `SystemConfig` (stack-light path).
 * @details Avoids large return envelopes in low-stack boot paths.
 * @param root Root JSON object of the candidate config payload.
 * @param outDecoded Receives decoded config on success.
 * @param outError Receives decode/shape error on failure.
 * @return `true` when decode succeeded.
 * @par Used By
 * - src/storage/storage_service.cpp
 */
bool decodeSystemConfigLight(JsonObjectConst root, SystemConfig& outDecoded,
                             ConfigValidationError& outError);

}  // namespace v3::storage


