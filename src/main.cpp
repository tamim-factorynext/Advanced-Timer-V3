#include <Arduino.h>
#include "core/platform/boot_diagnostics.hpp"
#include "core/platform/watchdog.hpp"
#include "core/runtime/runtime_types.hpp"
#include "core/runtime/service_manager.hpp"
#include "core/runtime/status_snapshot.hpp"
#include "core/services/telemetry_service.hpp"
#include "core/services/http_api_service.hpp"
#include "core/services/wifi_policy_service.hpp"

namespace {

constexpr const char *kBootBanner = "Advanced Timer V4 bootstrap (ESP32-S3)";
constexpr uint32_t kCore0PeriodMs = 10;
constexpr uint32_t kCore1PeriodMs = 50;
constexpr uint32_t kMetricsLogIntervalMs = 10000;

constexpr uint32_t kTaskWdtTimeoutSec = 16;
constexpr bool kExpectPsram = true;
constexpr size_t kPsramProbeBytes = 1 * 1024 * 1024;

const at::services::WifiPolicyConfig kWifiPolicyConfig = {
    "factorynext",
    "12345678",
    8000,
    "FactoryNext",
    "FactoryNext20$22#",
    12000,
    3000,
    60000,
};

TaskHandle_t gCore0TaskHandle = nullptr;
TaskHandle_t gCore1TaskHandle = nullptr;
portMUX_TYPE gStatsMux = portMUX_INITIALIZER_UNLOCKED;
at::runtime::LoopStats gCore0Stats;
at::runtime::LoopStats gCore1Stats;
at::runtime::StatusSnapshotModel gStatusSnapshotModel;

at::runtime::ServiceManager gServiceManager;
at::services::WifiPolicyService gWifiPolicyService(kWifiPolicyConfig);
at::services::TelemetryService gTelemetryService(
    gCore0Stats,
    gCore1Stats,
    gStatsMux,
    &gCore0TaskHandle,
    &gCore1TaskHandle,
    &gStatusSnapshotModel,
    kCore0PeriodMs,
    kCore1PeriodMs,
    kMetricsLogIntervalMs);
at::services::HttpApiService gHttpApiService(gStatusSnapshotModel);

void core0KernelTask(void * /*arg*/) {
  at::platform::addCurrentTaskToWatchdog();

  TickType_t lastWake = xTaskGetTickCount();
  constexpr uint32_t budgetUs = kCore0PeriodMs * 1000;

  for (;;) {
    const uint32_t t0 = micros();

    // Deterministic kernel scan placeholder (V4 baseline).
    // TODO(v4): execute card scan/runtime here.

    at::platform::feedCurrentTaskWatchdog();
    const uint32_t busyUs = micros() - t0;
    at::runtime::updateLoopStats(gCore0Stats, busyUs, budgetUs, gStatsMux);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(kCore0PeriodMs));
  }
}

void core1ServiceTask(void * /*arg*/) {
  at::platform::addCurrentTaskToWatchdog();

  TickType_t lastWake = xTaskGetTickCount();
  constexpr uint32_t budgetUs = kCore1PeriodMs * 1000;

  for (;;) {
    const uint32_t t0 = micros();

    gServiceManager.tickAll(millis());

    at::platform::feedCurrentTaskWatchdog();
    const uint32_t busyUs = micros() - t0;
    at::runtime::updateLoopStats(gCore1Stats, busyUs, budgetUs, gStatsMux);
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
  Serial.printf("[BOOT] Reset reason: %s (%d)\n",
                at::platform::resetReasonToString(rr),
                static_cast<int>(rr));
  at::platform::runPsramBootProbe(kExpectPsram, kPsramProbeBytes);

  at::platform::initTaskWatchdog(kTaskWdtTimeoutSec);

  gServiceManager.registerService(&gWifiPolicyService);
  gServiceManager.registerService(&gTelemetryService);
  gServiceManager.registerService(&gHttpApiService);
  gServiceManager.initAll();

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
