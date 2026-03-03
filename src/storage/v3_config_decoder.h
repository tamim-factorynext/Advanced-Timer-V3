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

struct ConfigDecodeResult {
  bool ok;
  ConfigValidationError error;
  SystemConfig decoded;
};

ConfigDecodeResult decodeSystemConfig(JsonObjectConst root);

}  // namespace v3::storage
