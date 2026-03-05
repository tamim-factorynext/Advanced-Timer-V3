#include "core/platform/boot_diagnostics.hpp"

#include <Arduino.h>

namespace at {
namespace platform {

const char *resetReasonToString(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_UNKNOWN: return "unknown";
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_EXT: return "external pin";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt watchdog";
    case ESP_RST_TASK_WDT: return "task watchdog";
    case ESP_RST_WDT: return "other watchdog";
    case ESP_RST_DEEPSLEEP: return "deep sleep wake";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
#ifdef ESP_RST_USB
    case ESP_RST_USB: return "usb";
#endif
#ifdef ESP_RST_JTAG
    case ESP_RST_JTAG: return "jtag";
#endif
#ifdef ESP_RST_EFUSE
    case ESP_RST_EFUSE: return "efuse";
#endif
#ifdef ESP_RST_PWR_GLITCH
    case ESP_RST_PWR_GLITCH: return "power glitch";
#endif
#ifdef ESP_RST_CPU_LOCKUP
    case ESP_RST_CPU_LOCKUP: return "cpu lockup";
#endif
    default: return "unmapped";
  }
}

void runPsramBootProbe(bool expectPsram, size_t probeBytes) {
  const bool hasPsram = psramFound();
  if (expectPsram && !hasPsram) {
    Serial.println("[BOOT][PSRAM] ERROR: Expected PSRAM but none detected.");
    return;
  }

  if (!hasPsram) {
    Serial.println("[BOOT][PSRAM] Not detected (allowed for this profile).");
    return;
  }

  void *probe = ps_malloc(probeBytes);
  if (!probe) {
    Serial.printf("[BOOT][PSRAM] ERROR: ps_malloc(%lu) failed.\n",
                  static_cast<unsigned long>(probeBytes));
    return;
  }

  memset(probe, 0xA5, probeBytes);
  free(probe);
  Serial.printf("[BOOT][PSRAM] OK: probe allocation passed (%lu bytes).\n",
                static_cast<unsigned long>(probeBytes));
}

}  // namespace platform
}  // namespace at

