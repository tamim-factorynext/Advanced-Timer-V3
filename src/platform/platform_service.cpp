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
#include <time.h>

namespace v3::platform {

void PlatformService::begin() {}

uint32_t PlatformService::nowMs() const { return millis(); }

bool PlatformService::readLocalMinuteStamp(LocalMinuteStamp& out) const {
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

void PlatformService::configureInputPin(uint8_t pin) const { pinMode(pin, INPUT); }

void PlatformService::configureOutputPin(uint8_t pin) const {
  pinMode(pin, OUTPUT);
}

bool PlatformService::readDigitalInput(uint8_t pin) const {
  return digitalRead(pin) != 0;
}

uint32_t PlatformService::readAnalogInput(uint8_t pin) const {
  return static_cast<uint32_t>(analogRead(pin));
}

void PlatformService::writeDigitalOutput(uint8_t pin, bool value) const {
  digitalWrite(pin, value ? HIGH : LOW);
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
