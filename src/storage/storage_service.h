#pragma once

#include "storage/v3_config_validator.h"

namespace v3::storage {

class StorageService {
 public:
  void begin();
  bool hasActiveConfig() const;
  const ValidatedConfig& activeConfig() const;
  const ConfigValidationError& lastError() const;

 private:
  ValidatedConfig activeConfig_ = {};
  ConfigValidationError lastError_ = {ConfigErrorCode::None, 0};
  bool activeConfigPresent_ = false;
};

}  // namespace v3::storage
