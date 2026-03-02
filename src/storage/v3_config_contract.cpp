#include "storage/v3_config_contract.h"

#include <string.h>

namespace v3::storage {

SystemConfig makeDefaultSystemConfig() {
  SystemConfig cfg = {};
  cfg.schemaVersion = kConfigSchemaVersion;
  cfg.scanIntervalMs = kMinScanIntervalMs;
  strncpy(cfg.wifi.master.ssid, "advancedtimer", sizeof(cfg.wifi.master.ssid) - 1);
  strncpy(cfg.wifi.master.password, "12345678",
          sizeof(cfg.wifi.master.password) - 1);
  cfg.wifi.master.timeoutSec = 2;
  cfg.wifi.master.editable = false;

  strncpy(cfg.wifi.user.ssid, "FactoryNext", sizeof(cfg.wifi.user.ssid) - 1);
  strncpy(cfg.wifi.user.password, "FactoryNext20$22#",
          sizeof(cfg.wifi.user.password) - 1);
  cfg.wifi.user.timeoutSec = 180;
  cfg.wifi.user.editable = true;

  cfg.wifi.retryBackoffSec = 30;
  cfg.wifi.staOnly = true;
  cfg.cardCount = 0;
  return cfg;
}

}  // namespace v3::storage
