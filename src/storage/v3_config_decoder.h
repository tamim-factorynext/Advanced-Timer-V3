/*
File: src/storage/v3_config_decoder.h
Purpose: Declares the v3 config decoder module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/storage/storage_service.cpp
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <ArduinoJson.h>

#include "storage/v3_config_contract.h"
#include "storage/v3_config_validator.h"

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

}  // namespace v3::storage
