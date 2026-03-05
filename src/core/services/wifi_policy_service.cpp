#include "core/services/wifi_policy_service.hpp"

#include <Arduino.h>

namespace at {
namespace services {

WifiPolicyService::WifiPolicyService(const WifiPolicyConfig &config) : config_(config) {}

namespace {
bool hasNetworkConfig(const char *ssid, const char *pass) {
  return ssid != nullptr && pass != nullptr && ssid[0] != '\0';
}
}  // namespace

const char *WifiPolicyService::name() const {
  return "wifi_policy";
}

void WifiPolicyService::init() {
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      Serial.printf("[WIFI][EVT] STA disconnected, reason=%d\n",
                    static_cast<int>(info.wifi_sta_disconnected.reason));
    }
  });
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_OFF);
  Serial.printf("[WIFI] Cool mode: radio cooldown=%lu ms, retry backoff=%lu ms\n",
                static_cast<unsigned long>(config_.radioCooldownMs),
                static_cast<unsigned long>(config_.offlineRetryDelayMs));
}

void WifiPolicyService::startWifiAttempt(const char *ssid, const char *pass, uint32_t nowMs) {
  WiFi.mode(WIFI_STA);
  // Keep modem sleep disabled during association/auth to improve join reliability.
  WiFi.setSleep(false);
  WiFi.begin(ssid, pass);
  stateStartedMs_ = nowMs;
  Serial.printf("[WIFI] Attempting connect: %s\n", ssid);
}

void WifiPolicyService::scheduleCooldownThen(State nextState, uint32_t nowMs) {
  WiFi.disconnect(false, true);
  WiFi.mode(WIFI_OFF);
  nextStateAfterCooldown_ = nextState;
  state_ = State::RadioCooldown;
  stateStartedMs_ = nowMs;
}

void WifiPolicyService::tick(uint32_t nowMs) {
  switch (state_) {
    case State::IdleStartBackup:
      if (hasNetworkConfig(config_.backupSsid, config_.backupPass)) {
        startWifiAttempt(config_.backupSsid, config_.backupPass, nowMs);
        state_ = State::WaitingBackup;
      } else {
        state_ = State::StartUser;
      }
      break;

    case State::WaitingBackup:
      if (WiFi.status() == WL_CONNECTED) {
        WiFi.setSleep(true);
        Serial.printf("[WIFI] Connected via backup. IP=%s RSSI=%d dBm\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
        state_ = State::Connected;
      } else if (nowMs - stateStartedMs_ >= config_.backupTimeoutMs) {
        scheduleCooldownThen(State::StartUser, nowMs);
      }
      break;

    case State::StartUser:
      if (hasNetworkConfig(config_.userSsid, config_.userPass)) {
        startWifiAttempt(config_.userSsid, config_.userPass, nowMs);
        state_ = State::WaitingUser;
      } else {
        state_ = State::OfflineBackoff;
        stateStartedMs_ = nowMs;
        Serial.println("[WIFI] No user SSID configured, entering offline mode.");
      }
      break;

    case State::WaitingUser:
      if (WiFi.status() == WL_CONNECTED) {
        WiFi.setSleep(true);
        Serial.printf("[WIFI] Connected via user network. IP=%s RSSI=%d dBm\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
        state_ = State::Connected;
      } else if (nowMs - stateStartedMs_ >= config_.userTimeoutMs) {
        state_ = State::OfflineBackoff;
        stateStartedMs_ = nowMs;
        WiFi.disconnect(false, true);
        WiFi.mode(WIFI_OFF);
        Serial.println("[WIFI] Entering offline mode (will retry in background).");
      }
      break;

    case State::Connected:
      if (WiFi.status() != WL_CONNECTED) {
        state_ = State::OfflineBackoff;
        stateStartedMs_ = nowMs;
        WiFi.disconnect(false, true);
        WiFi.mode(WIFI_OFF);
        Serial.println("[WIFI] Link lost, entering offline backoff.");
      }
      break;

    case State::RadioCooldown:
      if (nowMs - stateStartedMs_ >= config_.radioCooldownMs) {
        state_ = nextStateAfterCooldown_;
      }
      break;

    case State::OfflineBackoff:
      if (nowMs - stateStartedMs_ >= config_.offlineRetryDelayMs) {
        state_ = State::IdleStartBackup;
      }
      break;
  }
}

}  // namespace services
}  // namespace at
