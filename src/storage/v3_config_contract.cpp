#include "storage/v3_config_contract.h"

namespace v3::storage {

SystemConfig makeDefaultSystemConfig() {
  SystemConfig cfg = {};
  cfg.schemaVersion = kConfigSchemaVersion;
  cfg.scanIntervalMs = kMinScanIntervalMs;
  cfg.cardCount = 0;
  return cfg;
}

}  // namespace v3::storage
