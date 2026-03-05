#include <Arduino.h>
#include <WiFi.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>
#include <esp_system.h>

/**
 * V4 baseline bring-up scaffold:
 * - Core0: deterministic kernel loop placeholder
 * - Core1: service loop (Wi-Fi policy + metrics/logging)
 * - Task watchdog for both loops
 * - Baseline performance/resource telemetry
 */

namespace {

constexpr const char *kBootBanner = "Advanced Timer V4 bootstrap (ESP32-S3)";

constexpr uint32_t kCore0PeriodMs = 10;
constexpr uint32_t kCore1PeriodMs = 50;
constexpr uint32_t kMetricsLogIntervalMs = 10000;

constexpr uint32_t kTaskWdtTimeoutSec = 16;
constexpr bool kExpectPsram = true;
constexpr size_t kPsramProbeBytes = 1 * 1024 * 1024;

// Wi-Fi policy baseline (backup first, then user, then offline retry).
constexpr const char *kBackupSsid = "factorynext";
constexpr const char *kBackupPass = "12345678";
constexpr uint32_t kBackupTimeoutMs = 8000;

constexpr const char *kUserSsid = "FactoryNext";
constexpr const char *kUserPass = "FactoryNext20$22#";
constexpr uint32_t kUserTimeoutMs = 12000;

constexpr uint32_t kOfflineRetryDelayMs = 60000;
constexpr uint32_t kWifiRadioCooldownMs = 3000;

TaskHandle_t gCore0TaskHandle = nullptr;
TaskHandle_t gCore1TaskHandle = nullptr;

portMUX_TYPE gStatsMux = portMUX_INITIALIZER_UNLOCKED;

struct LoopStats {
  uint64_t cycles = 0;
  uint64_t totalBusyUs = 0;
  uint32_t maxBusyUs = 0;
  uint32_t overrunCount = 0;
};

LoopStats gCore0Stats;
LoopStats gCore1Stats;

enum class WifiPolicyState : uint8_t {
  IdleStartBackup,
  WaitingBackup,
  StartUser,
  WaitingUser,
  Connected,
  RadioCooldown,
  OfflineBackoff
};

WifiPolicyState gWifiState = WifiPolicyState::IdleStartBackup;
uint32_t gWifiStateStartedMs = 0;
WifiPolicyState gWifiNextStateAfterCooldown = WifiPolicyState::IdleStartBackup;

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

void updateLoopStats(LoopStats &stats, uint32_t busyUs, uint32_t budgetUs) {
  portENTER_CRITICAL(&gStatsMux);
  stats.cycles++;
  stats.totalBusyUs += busyUs;
  if (busyUs > stats.maxBusyUs) {
    stats.maxBusyUs = busyUs;
  }
  if (busyUs > budgetUs) {
    stats.overrunCount++;
  }
  portEXIT_CRITICAL(&gStatsMux);
}

void initTaskWatchdog() {
#if ESP_IDF_VERSION_MAJOR >= 5
  esp_task_wdt_config_t twdtConfig = {
      .timeout_ms = kTaskWdtTimeoutSec * 1000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&twdtConfig);
#else
  esp_task_wdt_init(kTaskWdtTimeoutSec, true);
#endif
}

void addCurrentTaskToWatchdog() {
  esp_task_wdt_add(nullptr);
}

void feedCurrentTaskWatchdog() {
  esp_task_wdt_reset();
}

void logMetricsSnapshot() {
  LoopStats core0Copy;
  LoopStats core1Copy;
  portENTER_CRITICAL(&gStatsMux);
  core0Copy = gCore0Stats;
  core1Copy = gCore1Stats;
  portEXIT_CRITICAL(&gStatsMux);

  const uint32_t core0BudgetUs = kCore0PeriodMs * 1000;
  const uint32_t core1BudgetUs = kCore1PeriodMs * 1000;
  const uint32_t core0AvgUs =
      core0Copy.cycles ? static_cast<uint32_t>(core0Copy.totalBusyUs / core0Copy.cycles) : 0;
  const uint32_t core1AvgUs =
      core1Copy.cycles ? static_cast<uint32_t>(core1Copy.totalBusyUs / core1Copy.cycles) : 0;

  const float core0LoadPct = (core0BudgetUs > 0) ? (100.0f * core0AvgUs / core0BudgetUs) : 0.0f;
  const float core1LoadPct = (core1BudgetUs > 0) ? (100.0f * core1AvgUs / core1BudgetUs) : 0.0f;

  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t minFreeHeap = ESP.getMinFreeHeap();
  const bool hasPsram = psramFound();
  const uint32_t freePsram = hasPsram ? ESP.getFreePsram() : 0;
  const uint32_t minFreePsram = hasPsram ? ESP.getMinFreePsram() : 0;

  const UBaseType_t core0StackWords =
      gCore0TaskHandle ? uxTaskGetStackHighWaterMark(gCore0TaskHandle) : 0;
  const UBaseType_t core1StackWords =
      gCore1TaskHandle ? uxTaskGetStackHighWaterMark(gCore1TaskHandle) : 0;
  Serial.printf(
      "[METRICS] C0(avg/max/ovr): %lu/%lu/%lu us load=%.1f%% | "
      "C1(avg/max/ovr): %lu/%lu/%lu us load=%.1f%%\n",
      static_cast<unsigned long>(core0AvgUs),
      static_cast<unsigned long>(core0Copy.maxBusyUs),
      static_cast<unsigned long>(core0Copy.overrunCount),
      core0LoadPct,
      static_cast<unsigned long>(core1AvgUs),
      static_cast<unsigned long>(core1Copy.maxBusyUs),
      static_cast<unsigned long>(core1Copy.overrunCount),
      core1LoadPct);

  Serial.printf(
      "[MEM] heap free/min: %lu/%lu | psram present: %s free/min: %lu/%lu | "
      "stackHW words C0/C1: %lu/%lu\n",
      static_cast<unsigned long>(freeHeap),
      static_cast<unsigned long>(minFreeHeap),
      hasPsram ? "yes" : "no",
      static_cast<unsigned long>(freePsram),
      static_cast<unsigned long>(minFreePsram),
      static_cast<unsigned long>(core0StackWords),
      static_cast<unsigned long>(core1StackWords));
}

void runPsramBootProbe() {
  const bool hasPsram = psramFound();
  if (kExpectPsram && !hasPsram) {
    Serial.println("[BOOT][PSRAM] ERROR: Expected PSRAM but none detected.");
    return;
  }

  if (!hasPsram) {
    Serial.println("[BOOT][PSRAM] Not detected (allowed for this profile).");
    return;
  }

  void *probe = ps_malloc(kPsramProbeBytes);
  if (!probe) {
    Serial.printf("[BOOT][PSRAM] ERROR: ps_malloc(%lu) failed.\n",
                  static_cast<unsigned long>(kPsramProbeBytes));
    return;
  }

  memset(probe, 0xA5, kPsramProbeBytes);
  free(probe);
  Serial.printf("[BOOT][PSRAM] OK: probe allocation passed (%lu bytes).\n",
                static_cast<unsigned long>(kPsramProbeBytes));
}

void startWifiAttempt(const char *ssid, const char *pass) {
  WiFi.mode(WIFI_STA);
  // Keep RF power draw lower while trying to connect on marginal hardware.
  WiFi.setSleep(true);
  WiFi.begin(ssid, pass);
  gWifiStateStartedMs = millis();
  Serial.printf("[WIFI] Attempting connect: %s\n", ssid);
}

void scheduleWifiCooldownThen(WifiPolicyState nextState) {
  WiFi.disconnect(false, true);
  WiFi.mode(WIFI_OFF);
  gWifiNextStateAfterCooldown = nextState;
  gWifiState = WifiPolicyState::RadioCooldown;
  gWifiStateStartedMs = millis();
}

void runWifiPolicyTick() {
  switch (gWifiState) {
    case WifiPolicyState::IdleStartBackup:
      startWifiAttempt(kBackupSsid, kBackupPass);
      gWifiState = WifiPolicyState::WaitingBackup;
      break;

    case WifiPolicyState::WaitingBackup:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected via backup. IP=%s RSSI=%d dBm\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
        gWifiState = WifiPolicyState::Connected;
      } else if (millis() - gWifiStateStartedMs >= kBackupTimeoutMs) {
        scheduleWifiCooldownThen(WifiPolicyState::StartUser);
      }
      break;

    case WifiPolicyState::StartUser:
      startWifiAttempt(kUserSsid, kUserPass);
      gWifiState = WifiPolicyState::WaitingUser;
      break;

    case WifiPolicyState::WaitingUser:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected via user network. IP=%s RSSI=%d dBm\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
        gWifiState = WifiPolicyState::Connected;
      } else if (millis() - gWifiStateStartedMs >= kUserTimeoutMs) {
        gWifiState = WifiPolicyState::OfflineBackoff;
        gWifiStateStartedMs = millis();
        WiFi.disconnect(false, true);
        WiFi.mode(WIFI_OFF);
        Serial.println("[WIFI] Entering offline mode (will retry in background).");
      }
      break;

    case WifiPolicyState::Connected:
      if (WiFi.status() != WL_CONNECTED) {
        gWifiState = WifiPolicyState::OfflineBackoff;
        gWifiStateStartedMs = millis();
        WiFi.disconnect(false, true);
        WiFi.mode(WIFI_OFF);
        Serial.println("[WIFI] Link lost, entering offline backoff.");
      }
      break;

    case WifiPolicyState::RadioCooldown:
      if (millis() - gWifiStateStartedMs >= kWifiRadioCooldownMs) {
        gWifiState = gWifiNextStateAfterCooldown;
      }
      break;

    case WifiPolicyState::OfflineBackoff:
      if (millis() - gWifiStateStartedMs >= kOfflineRetryDelayMs) {
        gWifiState = WifiPolicyState::IdleStartBackup;
      }
      break;
  }
}

void core0KernelTask(void * /*arg*/) {
  addCurrentTaskToWatchdog();

  TickType_t lastWake = xTaskGetTickCount();
  constexpr uint32_t budgetUs = kCore0PeriodMs * 1000;

  for (;;) {
    const uint32_t t0 = micros();

    // Deterministic kernel scan placeholder (V4 baseline).
    // TODO(v4): execute card scan/runtime here.

    feedCurrentTaskWatchdog();
    const uint32_t busyUs = micros() - t0;
    updateLoopStats(gCore0Stats, busyUs, budgetUs);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(kCore0PeriodMs));
  }
}

void core1ServiceTask(void * /*arg*/) {
  addCurrentTaskToWatchdog();

  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastMetricsLogMs = 0;
  constexpr uint32_t budgetUs = kCore1PeriodMs * 1000;

  for (;;) {
    const uint32_t t0 = micros();

    runWifiPolicyTick();
    if (millis() - lastMetricsLogMs >= kMetricsLogIntervalMs) {
      logMetricsSnapshot();
      lastMetricsLogMs = millis();
    }

    feedCurrentTaskWatchdog();
    const uint32_t busyUs = micros() - t0;
    updateLoopStats(gCore1Stats, busyUs, budgetUs);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(kCore1PeriodMs));
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.printf("%s\n", kBootBanner);
  Serial.println("[BOOT] Starting dual-core baseline...");
  const esp_reset_reason_t rr = esp_reset_reason();
  Serial.printf("[BOOT] Reset reason: %s (%d)\n", resetReasonToString(rr), static_cast<int>(rr));
  runPsramBootProbe();

  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_OFF);
  Serial.printf("[WIFI] Cool mode: radio cooldown=%lu ms, retry backoff=%lu ms\n",
                static_cast<unsigned long>(kWifiRadioCooldownMs),
                static_cast<unsigned long>(kOfflineRetryDelayMs));

  initTaskWatchdog();

  BaseType_t c0Ok = xTaskCreatePinnedToCore(
      core0KernelTask, "core0KernelTask", 4096, nullptr, 3, &gCore0TaskHandle, 0);
  BaseType_t c1Ok = xTaskCreatePinnedToCore(
      core1ServiceTask, "core1ServiceTask", 6144, nullptr, 2, &gCore1TaskHandle, 1);

  if (c0Ok != pdPASS || c1Ok != pdPASS) {
    Serial.printf("[BOOT] Task start failure c0=%ld c1=%ld\n",
                  static_cast<long>(c0Ok),
                  static_cast<long>(c1Ok));
  } else {
    Serial.println("[BOOT] Core tasks started (C0 kernel, C1 service).");
  }
}

void loop() {
  // Main loop remains idle; work is pinned to FreeRTOS core tasks.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
