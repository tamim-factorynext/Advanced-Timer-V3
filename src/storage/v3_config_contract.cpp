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

  strncpy(cfg.clock.timezone, "UTC", sizeof(cfg.clock.timezone) - 1);
  cfg.clock.ntp.enabled = true;
  strncpy(cfg.clock.ntp.primaryServer, "pool.ntp.org",
          sizeof(cfg.clock.ntp.primaryServer) - 1);
  strncpy(cfg.clock.ntp.secondaryServer, "time.google.com",
          sizeof(cfg.clock.ntp.secondaryServer) - 1);
  strncpy(cfg.clock.ntp.tertiaryServer, "time.cloudflare.com",
          sizeof(cfg.clock.ntp.tertiaryServer) - 1);
  cfg.clock.ntp.syncIntervalSec = 3600;
  cfg.clock.ntp.startupTimeoutSec = 15;
  cfg.clock.ntp.maxStaleSec = 86400;

  cfg.cardCount = 0;
  return cfg;
}

}  // namespace v3::storage
