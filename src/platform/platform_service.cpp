#include "platform/platform_service.h"

#include <Arduino.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>

namespace v3::platform {

void PlatformService::begin() {}

uint32_t PlatformService::nowMs() const { return millis(); }

void PlatformService::configureInputPin(uint8_t pin) const { pinMode(pin, INPUT); }

bool PlatformService::readDigitalInput(uint8_t pin) const {
  return digitalRead(pin) != 0;
}

uint32_t PlatformService::readAnalogInput(uint8_t pin) const {
  return static_cast<uint32_t>(analogRead(pin));
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
