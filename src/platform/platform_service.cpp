/*
File: src/platform/platform_service.cpp
Purpose: Implements the platform service module behavior.

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
#include "platform/platform_service.h"

#include <Arduino.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>
#include <RTClib.h>
#include <Wire.h>
#include <time.h>

namespace v3::platform {

namespace {

bool resolveChannelPin(uint8_t channel, const uint8_t* channels, uint8_t count,
                       uint8_t& outPin) {
  if (channels == nullptr || channel >= count) return false;
  outPin = channels[channel];
  return true;
}

RTC_DS3231 gRtcDs3231;
RTC_PCF8523 gRtcPcf8523;
RTC_DS1307 gRtcDs1307;
bool gRtcBackendReady = false;
constexpr bool kLogPlatformInitStages = true;

bool mapDateTimeToMinuteStamp(const DateTime& dt, LocalMinuteStamp& out) {
  if (!dt.isValid()) return false;
  out.year = static_cast<uint16_t>(dt.year());
  out.month = static_cast<uint8_t>(dt.month());
  out.day = static_cast<uint8_t>(dt.day());
  out.weekday = static_cast<uint8_t>(dt.dayOfTheWeek());
  out.hour = static_cast<uint8_t>(dt.hour());
  out.minute = static_cast<uint8_t>(dt.minute());
  return true;
}

}  // namespace

void PlatformService::begin() {
  auto logPlatformStage = [](const char* stage) {
    if (!kLogPlatformInitStages) return;
    Serial.println(stage);
    Serial.flush();
  };

  logPlatformStage("[platform] 01 begin()");
  profile_ = &activeHardwareProfile();
  gRtcBackendReady = false;
  if (profile_ == nullptr) {
    logPlatformStage("[platform] 02 profile null");
    return;
  }

  if (kLogPlatformInitStages) {
    Serial.printf("[platform] 03 rtcBackend=%u\n",
                  static_cast<unsigned>(profile_->rtcBackend));
    Serial.flush();
  }

  if (profile_->rtcBackend == RtcBackend::RtcMillis) {
    // Keep current behavior: RTC minute stamp source comes from system local time.
    gRtcBackendReady = true;
    logPlatformStage("[platform] 04 RTC_MILLIS selected");
    return;
  }

  logPlatformStage("[platform] 05 Wire.begin()");
  Wire.begin();
  logPlatformStage("[platform] 06 Wire.begin done");
  switch (profile_->rtcBackend) {
    case RtcBackend::Ds3231:
      logPlatformStage("[platform] 07 DS3231 begin()");
      gRtcBackendReady = gRtcDs3231.begin();
      break;
    case RtcBackend::Pcf8523:
      logPlatformStage("[platform] 08 PCF8523 begin()");
      gRtcBackendReady = gRtcPcf8523.begin();
      break;
    case RtcBackend::Ds1307:
      logPlatformStage("[platform] 09 DS1307 begin()");
      gRtcBackendReady = gRtcDs1307.begin();
      break;
    case RtcBackend::RtcMillis:
    default:
      gRtcBackendReady = true;
      break;
  }
  if (kLogPlatformInitStages) {
    Serial.printf("[platform] 10 rtc backend ready=%u\n",
                  gRtcBackendReady ? 1U : 0U);
    Serial.flush();
  }
}

uint32_t PlatformService::nowMs() const { return millis(); }

bool PlatformService::readLocalMinuteStamp(LocalMinuteStamp& out) const {
  if (profile_ == nullptr) return false;
  if (!gRtcBackendReady) return false;

  switch (profile_->rtcBackend) {
    case RtcBackend::Ds3231:
      return mapDateTimeToMinuteStamp(gRtcDs3231.now(), out);
    case RtcBackend::Pcf8523:
      return mapDateTimeToMinuteStamp(gRtcPcf8523.now(), out);
    case RtcBackend::Ds1307:
      return mapDateTimeToMinuteStamp(gRtcDs1307.now(), out);
    case RtcBackend::RtcMillis:
    default:
      break;
  }

  struct tm timeinfo = {};
  if (!getLocalTime(&timeinfo, 0)) return false;
  out.year = static_cast<uint16_t>(timeinfo.tm_year + 1900);
  out.month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
  out.day = static_cast<uint8_t>(timeinfo.tm_mday);
  out.weekday = static_cast<uint8_t>(timeinfo.tm_wday);
  out.hour = static_cast<uint8_t>(timeinfo.tm_hour);
  out.minute = static_cast<uint8_t>(timeinfo.tm_min);
  return true;
}

void PlatformService::configureDiChannel(uint8_t channel) const {
  if (profile_ == nullptr) return;
  uint8_t pin = 0;
  if (!resolveChannelPin(channel, profile_->diChannels, profile_->diCount, pin)) {
    return;
  }
  pinMode(pin, INPUT);
}

void PlatformService::configureAiChannel(uint8_t channel) const {
  if (profile_ == nullptr) return;
  uint8_t pin = 0;
  if (!resolveChannelPin(channel, profile_->aiChannels, profile_->aiCount, pin)) {
    return;
  }
  pinMode(pin, INPUT);
}

void PlatformService::configureDoChannel(uint8_t channel) const {
  if (profile_ == nullptr) return;
  uint8_t pin = 0;
  if (!resolveChannelPin(channel, profile_->doChannels, profile_->doCount, pin)) {
    return;
  }
  pinMode(pin, OUTPUT);
}

bool PlatformService::readDiChannel(uint8_t channel) const {
  if (profile_ == nullptr) return false;
  uint8_t pin = 0;
  if (!resolveChannelPin(channel, profile_->diChannels, profile_->diCount, pin)) {
    return false;
  }
  return digitalRead(pin) != 0;
}

uint32_t PlatformService::readAiChannel(uint8_t channel) const {
  if (profile_ == nullptr) return 0;
  uint8_t pin = 0;
  if (!resolveChannelPin(channel, profile_->aiChannels, profile_->aiCount, pin)) {
    return 0;
  }
  return static_cast<uint32_t>(analogRead(pin));
}

void PlatformService::writeDoChannel(uint8_t channel, bool value) const {
  if (profile_ == nullptr) return;
  uint8_t pin = 0;
  if (!resolveChannelPin(channel, profile_->doChannels, profile_->doCount, pin)) {
    return;
  }
  digitalWrite(pin, value ? HIGH : LOW);
}

const HardwareProfile& PlatformService::profile() const {
  if (profile_ != nullptr) return *profile_;
  return activeHardwareProfile();
}

bool PlatformService::initTaskWatchdog(uint32_t timeoutSeconds,
                                       bool panicOnTrigger) {
#if ESP_IDF_VERSION_MAJOR >= 5
  esp_task_wdt_config_t config = {};
  config.timeout_ms = timeoutSeconds * 1000;
  config.idle_core_mask = 0;
  config.trigger_panic = panicOnTrigger;
  esp_err_t err = esp_task_wdt_init(&config);
  return err == ESP_OK || err == ESP_ERR_INVALID_STATE;
#else
  esp_err_t err = esp_task_wdt_init(timeoutSeconds, panicOnTrigger);
  return err == ESP_OK || err == ESP_ERR_INVALID_STATE;
#endif
}

bool PlatformService::addCurrentTaskToWatchdog() const {
  esp_err_t err = esp_task_wdt_add(nullptr);
  return err == ESP_OK || err == ESP_ERR_INVALID_STATE;
}

bool PlatformService::resetTaskWatchdog() const {
  return esp_task_wdt_reset() == ESP_OK;
}

}  // namespace v3::platform
