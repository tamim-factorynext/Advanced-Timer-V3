/*
File: src/platform/wifi_runtime.cpp
Purpose: Implements the wifi runtime module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/main.cpp
Flow Hook:
- Board/platform integration and connectivity runtime.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "platform/wifi_runtime.h"

#include <WiFi.h>
#include <cstdio>
#include <cstring>
#include <time.h>

namespace v3::platform {

namespace {

void formatIp(const IPAddress& ip, char out[16]) {
  std::snprintf(out, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

const char* resolvePosixTimezone(const char* configuredTz) {
  // configTzTime expects POSIX TZ strings; map known IANA labels used in config.
  if (configuredTz == nullptr || configuredTz[0] == '\0') {
    return "UTC0";
  }
  if (std::strcmp(configuredTz, "Asia/Dhaka") == 0) {
    return "BDT-6";
  }
  return configuredTz;
}

}  // namespace

void WiFiRuntime::begin(const v3::storage::WiFiConfig& config,
                        const v3::storage::ClockConfig& clockConfig) {
  config_ = config;
  clockConfig_ = clockConfig;
  status_ = {};
  status_.state = WiFiState::Offline;
  status_.phase = WiFiConnectPhase::None;
  status_.online = false;
  timeSyncConfigured_ = false;
  lastTimeSyncMs_ = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  startBackupAccessNetworkAttempt(0);
}

void WiFiRuntime::tick(uint32_t nowMs) {
  if (WiFi.status() == WL_CONNECTED) {
    const bool becameConnected = (status_.state != WiFiState::StaConnected);
    status_.state = WiFiState::StaConnected;
    status_.phase = WiFiConnectPhase::None;
    status_.staConnected = true;
    status_.online = true;
    status_.offlineSinceMs = 0;
    refreshStaIp();

    if (clockConfig_.timeSync.enabled) {
      if (becameConnected || !timeSyncConfigured_) {
        configureTimeSync();
        lastTimeSyncMs_ = nowMs;
      } else if ((nowMs - lastTimeSyncMs_) >=
                 (clockConfig_.timeSync.syncIntervalSec * 1000UL)) {
        configureTimeSync();
        lastTimeSyncMs_ = nowMs;
      }
    }
    return;
  }

  status_.staConnected = false;
  status_.online = false;
  status_.staIp[0] = '\0';

  if (status_.state == WiFiState::StaConnected) {
    startBackupAccessNetworkAttempt(nowMs);
    return;
  }

  if (status_.state == WiFiState::ConnectingBackupAccessNetwork &&
      (nowMs - status_.phaseStartedMs) >=
          (config_.backupAccessNetwork.timeoutSec * 1000UL)) {
    startUserConfiguredNetworkAttempt(nowMs);
    return;
  }

  if (status_.state == WiFiState::ConnectingUserConfiguredNetwork &&
      (nowMs - status_.phaseStartedMs) >=
          (config_.userConfiguredNetwork.timeoutSec * 1000UL)) {
    enterOffline(nowMs);
    return;
  }

  if (status_.state == WiFiState::Offline &&
      (nowMs - status_.lastAttemptMs) >= (config_.retryDelaySec * 1000UL)) {
    startBackupAccessNetworkAttempt(nowMs);
    return;
  }
}

const WiFiStatus& WiFiRuntime::status() const { return status_; }

void WiFiRuntime::startBackupAccessNetworkAttempt(uint32_t nowMs) {
  ++status_.retryCount;
  status_.lastAttemptMs = nowMs;
  status_.phaseStartedMs = nowMs;
  status_.state = WiFiState::ConnectingBackupAccessNetwork;
  status_.phase = WiFiConnectPhase::BackupAccessNetwork;
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_.backupAccessNetwork.ssid,
             config_.backupAccessNetwork.password);
}

void WiFiRuntime::configureTimeSync() {
  if (!clockConfig_.timeSync.enabled) return;
  configTzTime(resolvePosixTimezone(clockConfig_.timezone),
               clockConfig_.timeSync.primaryTimeServer);
  timeSyncConfigured_ = true;
}

void WiFiRuntime::startUserConfiguredNetworkAttempt(uint32_t nowMs) {
  status_.lastAttemptMs = nowMs;
  status_.phaseStartedMs = nowMs;
  status_.state = WiFiState::ConnectingUserConfiguredNetwork;
  status_.phase = WiFiConnectPhase::UserConfiguredNetwork;
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_.userConfiguredNetwork.ssid,
             config_.userConfiguredNetwork.password);
}

void WiFiRuntime::enterOffline(uint32_t nowMs) {
  WiFi.disconnect(false, false);
  status_.lastAttemptMs = nowMs;
  status_.phaseStartedMs = nowMs;
  status_.state = WiFiState::Offline;
  status_.phase = WiFiConnectPhase::None;
  status_.online = false;
  if (status_.offlineSinceMs == 0) {
    status_.offlineSinceMs = nowMs;
  }
  status_.staIp[0] = '\0';
}

void WiFiRuntime::refreshStaIp() {
  if (!status_.staConnected) {
    status_.staIp[0] = '\0';
    return;
  }
  formatIp(WiFi.localIP(), status_.staIp);
}

}  // namespace v3::platform

