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
#include <cstdlib>
#include <cstring>
#include <time.h>

#include "storage/v3_timezones.h"

namespace v3::platform {

namespace {

constexpr bool kLogTimeSync = false;
constexpr int kMinimumSyncedYear = 2024;
constexpr const char* kUtcTimezone = "UTC0";
constexpr uint32_t kTimeSyncConnectDelayMs = 1000;

void formatIp(const IPAddress& ip, char out[16]) {
  std::snprintf(out, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
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
  lastTimeValid_ = false;
  connectedSinceMs_ = 0;
  lastTimeSyncMs_ = 0;
  applyLocalTimezone();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  startBackupAccessNetworkAttempt(0);
}

void WiFiRuntime::tick(uint32_t nowMs) {
  if (WiFi.status() == WL_CONNECTED) {
    const bool becameConnected = (status_.state != WiFiState::StaConnected);
    if (becameConnected)
      connectedSinceMs_ = nowMs;
    status_.state = WiFiState::StaConnected;
    status_.phase = WiFiConnectPhase::None;
    status_.staConnected = true;
    status_.online = true;
    status_.offlineSinceMs = 0;
    refreshStaIp();

    if (clockConfig_.timeSync.enabled) {
      const bool connectionStable =
          connectedSinceMs_ != 0 &&
          (nowMs - connectedSinceMs_) >= kTimeSyncConnectDelayMs;
      if (connectionStable && !timeSyncConfigured_) {
        configureTimeSync();
        lastTimeSyncMs_ = nowMs;
      } else if (connectionStable &&
                 (nowMs - lastTimeSyncMs_) >=
                     (clockConfig_.timeSync.syncIntervalSec * 1000UL)) {
        configureTimeSync();
        lastTimeSyncMs_ = nowMs;
      }

      const time_t epochNow = time(nullptr);
      struct tm localTime = {};
      const bool timeValid =
          (epochNow > 0) && (localtime_r(&epochNow, &localTime) != nullptr) &&
          ((localTime.tm_year + 1900) >= kMinimumSyncedYear);
      if (timeValid && !lastTimeValid_ && kLogTimeSync) {
        Serial.printf("[time-sync] valid epoch=%lu year=%d ip=%s\n",
                      static_cast<unsigned long>(epochNow),
                      localTime.tm_year + 1900, status_.staIp);
        Serial.flush();
        lastTimeValid_ = timeValid;
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
  connectedSinceMs_ = 0;
  timeSyncConfigured_ = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_.backupAccessNetwork.ssid,
             config_.backupAccessNetwork.password);
}

void WiFiRuntime::applyLocalTimezone() const {
  const char *posixTz =
      v3::storage::resolvePosixTimezone(clockConfig_.timezone);
  setenv("TZ", posixTz, 1);
  tzset();
  if (kLogTimeSync) {
    Serial.printf("[time-sync] local timezone configured id=%s posix=%s\n",
                  clockConfig_.timezone, posixTz);
    Serial.flush();
  }
}

void WiFiRuntime::configureTimeSync() {
  if (!clockConfig_.timeSync.enabled) return;
  const time_t epochBefore = time(nullptr);
  if (kLogTimeSync) {
    Serial.printf(
        "[time-sync] configure server=%s tz=%s configuredTz=%s syncIntervalSec=%lu epochBefore=%ld\n",
        clockConfig_.timeSync.primaryTimeServer, kUtcTimezone,
        clockConfig_.timezone,
        static_cast<unsigned long>(clockConfig_.timeSync.syncIntervalSec),
        static_cast<long>(epochBefore));
    Serial.flush();
  }
  configTime(0, 0, clockConfig_.timeSync.primaryTimeServer);
  applyLocalTimezone();
  timeSyncConfigured_ = true;
}

void WiFiRuntime::startUserConfiguredNetworkAttempt(uint32_t nowMs) {
  status_.lastAttemptMs = nowMs;
  status_.phaseStartedMs = nowMs;
  status_.state = WiFiState::ConnectingUserConfiguredNetwork;
  status_.phase = WiFiConnectPhase::UserConfiguredNetwork;
  connectedSinceMs_ = 0;
  timeSyncConfigured_ = false;
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
  connectedSinceMs_ = 0;
  timeSyncConfigured_ = false;
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

