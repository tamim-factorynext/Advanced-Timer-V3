#include "platform/wifi_runtime.h"

#include <WiFi.h>
#include <cstdio>

namespace v3::platform {

namespace {

void formatIp(const IPAddress& ip, char out[16]) {
  std::snprintf(out, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

}  // namespace

void WiFiRuntime::begin(const v3::storage::WiFiConfig& config) {
  config_ = config;
  status_ = {};
  status_.state = WiFiState::Offline;
  status_.phase = WiFiConnectPhase::None;
  status_.online = false;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  startMasterAttempt(0);
}

void WiFiRuntime::tick(uint32_t nowMs) {
  if (WiFi.status() == WL_CONNECTED) {
    status_.state = WiFiState::StaConnected;
    status_.phase = WiFiConnectPhase::None;
    status_.staConnected = true;
    status_.online = true;
    status_.offlineSinceMs = 0;
    refreshStaIp();
    return;
  }

  status_.staConnected = false;
  status_.online = false;
  status_.staIp[0] = '\0';

  if (status_.state == WiFiState::StaConnected) {
    startMasterAttempt(nowMs);
    return;
  }

  if (status_.state == WiFiState::ConnectingMaster &&
      (nowMs - status_.phaseStartedMs) >= (config_.master.timeoutSec * 1000UL)) {
    startUserAttempt(nowMs);
    return;
  }

  if (status_.state == WiFiState::ConnectingUser &&
      (nowMs - status_.phaseStartedMs) >= (config_.user.timeoutSec * 1000UL)) {
    enterOffline(nowMs);
    return;
  }

  if (status_.state == WiFiState::Offline &&
      (nowMs - status_.lastAttemptMs) >= (config_.retryBackoffSec * 1000UL)) {
    startMasterAttempt(nowMs);
    return;
  }
}

const WiFiStatus& WiFiRuntime::status() const { return status_; }

void WiFiRuntime::startMasterAttempt(uint32_t nowMs) {
  ++status_.retryCount;
  status_.lastAttemptMs = nowMs;
  status_.phaseStartedMs = nowMs;
  status_.state = WiFiState::ConnectingMaster;
  status_.phase = WiFiConnectPhase::Master;
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_.master.ssid, config_.master.password);
}

void WiFiRuntime::startUserAttempt(uint32_t nowMs) {
  status_.lastAttemptMs = nowMs;
  status_.phaseStartedMs = nowMs;
  status_.state = WiFiState::ConnectingUser;
  status_.phase = WiFiConnectPhase::User;
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_.user.ssid, config_.user.password);
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
