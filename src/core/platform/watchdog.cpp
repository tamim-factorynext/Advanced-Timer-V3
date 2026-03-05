#include "core/platform/watchdog.hpp"

#include <esp_idf_version.h>
#include <esp_task_wdt.h>

namespace at {
namespace platform {

void initTaskWatchdog(uint32_t timeoutSec) {
#if ESP_IDF_VERSION_MAJOR >= 5
  esp_task_wdt_config_t twdtConfig = {
      .timeout_ms = timeoutSec * 1000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&twdtConfig);
#else
  esp_task_wdt_init(timeoutSec, true);
#endif
}

void addCurrentTaskToWatchdog() {
  esp_task_wdt_add(nullptr);
}

void feedCurrentTaskWatchdog() {
  esp_task_wdt_reset();
}

}  // namespace platform
}  // namespace at

