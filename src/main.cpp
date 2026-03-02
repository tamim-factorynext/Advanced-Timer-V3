#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "control/control_service.h"
#include "kernel/kernel_service.h"
#include "platform/platform_service.h"
#include "portal/portal_service.h"
#include "runtime/runtime_service.h"
#include "storage/storage_service.h"

namespace {

v3::platform::PlatformService gPlatform;
v3::storage::StorageService gStorage;
v3::kernel::KernelService gKernel;
v3::runtime::RuntimeService gRuntime;
v3::control::ControlService gControl;
v3::portal::PortalService gPortal;
v3::storage::BootstrapDiagnostics gBootstrapDiagnostics = {};
v3::kernel::KernelMetrics gSharedKernelMetrics = {};
portMUX_TYPE gKernelMetricsMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t gCore0TaskHandle = nullptr;
TaskHandle_t gCore1TaskHandle = nullptr;

constexpr uint32_t kCore0LoopDelayMs = 1;
constexpr uint32_t kCore1LoopDelayMs = 1;

v3::kernel::KernelMetrics copySharedKernelMetrics() {
  v3::kernel::KernelMetrics copy = {};
  portENTER_CRITICAL(&gKernelMetricsMux);
  copy = gSharedKernelMetrics;
  portEXIT_CRITICAL(&gKernelMetricsMux);
  return copy;
}

void publishSharedKernelMetrics(const v3::kernel::KernelMetrics& metrics) {
  portENTER_CRITICAL(&gKernelMetricsMux);
  gSharedKernelMetrics = metrics;
  portEXIT_CRITICAL(&gKernelMetricsMux);
}

void core0KernelTask(void*) {
  while (true) {
    const uint32_t nowMs = gPlatform.nowMs();
    gKernel.tick(nowMs);
    publishSharedKernelMetrics(gKernel.metrics());
    vTaskDelay(pdMS_TO_TICKS(kCore0LoopDelayMs));
  }
}

void core1ServiceTask(void*) {
  while (true) {
    const uint32_t nowMs = gPlatform.nowMs();
    const v3::kernel::KernelMetrics metrics = copySharedKernelMetrics();

    gControl.tick(nowMs);
    gRuntime.tick(nowMs, metrics, gBootstrapDiagnostics);
    gPortal.tick(nowMs, gRuntime.snapshot());

    vTaskDelay(pdMS_TO_TICKS(kCore1LoopDelayMs));
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Advanced Timer V3 Core bootstrap");

  gPlatform.begin();
  gStorage.begin();

  if (!gStorage.hasActiveConfig()) {
    const v3::storage::ConfigValidationError err = gStorage.lastError();
    Serial.print("V3 config bootstrap failed: ");
    Serial.println(v3::storage::configErrorCodeToString(err.code));
    while (true) {
      delay(1000);
    }
  }

  gKernel.begin(gStorage.activeConfig());
  gBootstrapDiagnostics = gStorage.diagnostics();
  gSharedKernelMetrics = gKernel.metrics();

  gRuntime.begin();
  gControl.begin();
  gPortal.begin();

  BaseType_t core0Created = xTaskCreatePinnedToCore(
      core0KernelTask, "v3-core0-kernel", 4096, nullptr, 2, &gCore0TaskHandle, 0);
  BaseType_t core1Created = xTaskCreatePinnedToCore(
      core1ServiceTask, "v3-core1-services", 6144, nullptr, 1, &gCore1TaskHandle,
      1);

  if (core0Created != pdPASS || core1Created != pdPASS) {
    Serial.println("V3 dual-core task bootstrap failed");
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
