/*
File: src/storage/v3_config_contract.cpp
Purpose: Implements the v3 config contract module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/storage/storage_service.cpp
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "storage/v3_config_contract.h"

#include <string.h>

namespace v3::storage {

SystemConfig makeDefaultSystemConfig() {
  SystemConfig cfg = {};
  cfg.schemaVersion = kConfigSchemaVersion;
  cfg.scanPeriodMs = kMinScanIntervalMs;
  strncpy(cfg.wifi.backupAccessNetwork.ssid, "advancedtimer", sizeof(cfg.wifi.backupAccessNetwork.ssid) - 1);
  strncpy(cfg.wifi.backupAccessNetwork.password, "12345678",
          sizeof(cfg.wifi.backupAccessNetwork.password) - 1);
  cfg.wifi.backupAccessNetwork.timeoutSec = 2;
  cfg.wifi.backupAccessNetwork.editable = false;

  strncpy(cfg.wifi.userConfiguredNetwork.ssid, "FactoryNext", sizeof(cfg.wifi.userConfiguredNetwork.ssid) - 1);
  strncpy(cfg.wifi.userConfiguredNetwork.password, "FactoryNext20$22#",
          sizeof(cfg.wifi.userConfiguredNetwork.password) - 1);
  cfg.wifi.userConfiguredNetwork.timeoutSec = 180;
  cfg.wifi.userConfiguredNetwork.editable = true;

  cfg.wifi.retryDelaySec = 30;
  cfg.wifi.staOnly = true;

  strncpy(cfg.time.timezone, "Asia/Dhaka", sizeof(cfg.time.timezone) - 1);
  cfg.time.timeSync.enabled = true;
  strncpy(cfg.time.timeSync.primaryTimeServer, "pool.ntp.org",
          sizeof(cfg.time.timeSync.primaryTimeServer) - 1);
  strncpy(cfg.time.timeSync.secondaryServer, "time.google.com",
          sizeof(cfg.time.timeSync.secondaryServer) - 1);
  strncpy(cfg.time.timeSync.tertiaryServer, "time.cloudflare.com",
          sizeof(cfg.time.timeSync.tertiaryServer) - 1);
  cfg.time.timeSync.syncIntervalSec = 3600;
  cfg.time.timeSync.startupTimeoutSec = 15;
  cfg.time.timeSync.maxTimeAgeSec = 86400;

  cfg.cardCount = 0;
  return cfg;
}

}  // namespace v3::storage



