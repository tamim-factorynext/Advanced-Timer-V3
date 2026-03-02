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
