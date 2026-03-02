#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
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
v3::kernel::KernelMetrics gLastKernelMetrics = {};
TaskHandle_t gCore0TaskHandle = nullptr;
TaskHandle_t gCore1TaskHandle = nullptr;
QueueHandle_t gKernelSnapshotQueue = nullptr;
QueueHandle_t gKernelCommandQueue = nullptr;

uint16_t gKernelSnapshotQueueDepth = 0;
uint16_t gKernelSnapshotQueueHighWater = 0;
uint32_t gKernelSnapshotQueueDropCount = 0;
uint16_t gKernelCommandQueueDepth = 0;
uint16_t gKernelCommandQueueHighWater = 0;
uint32_t gKernelCommandQueueDropCount = 0;
uint32_t gKernelCommandAppliedCount = 0;
uint32_t gKernelCommandNoopCount = 0;
uint32_t gKernelCommandLastLatencyMs = 0;
uint32_t gKernelCommandMaxLatencyMs = 0;

constexpr uint32_t kCore0LoopDelayMs = 1;
constexpr uint32_t kCore1LoopDelayMs = 1;
constexpr uint8_t kKernelSnapshotQueueCapacity = 8;
constexpr uint8_t kKernelCommandQueueCapacity = 16;
constexpr uint32_t kCommandHeartbeatIntervalMs = 250;

enum class KernelCommandType : uint8_t {
  NoopHeartbeat = 0,
};

struct KernelCommand {
  KernelCommandType type;
  uint32_t sequence;
  uint32_t enqueuedAtMs;
};

struct KernelSnapshotMessage {
  uint32_t producedAtMs;
  v3::kernel::KernelMetrics metrics;
};

void updateKernelSnapshotQueueStats() {
  if (gKernelSnapshotQueue == nullptr) return;
  gKernelSnapshotQueueDepth =
      static_cast<uint16_t>(uxQueueMessagesWaiting(gKernelSnapshotQueue));
  if (gKernelSnapshotQueueDepth > gKernelSnapshotQueueHighWater) {
    gKernelSnapshotQueueHighWater = gKernelSnapshotQueueDepth;
  }
}

void enqueueKernelSnapshot(const KernelSnapshotMessage& snapshot) {
  if (gKernelSnapshotQueue == nullptr) return;
  if (xQueueSend(gKernelSnapshotQueue, &snapshot, 0) != pdTRUE) {
    gKernelSnapshotQueueDropCount += 1;
  }
  updateKernelSnapshotQueueStats();
}

void updateKernelCommandQueueStats() {
  if (gKernelCommandQueue == nullptr) return;
  gKernelCommandQueueDepth =
      static_cast<uint16_t>(uxQueueMessagesWaiting(gKernelCommandQueue));
  if (gKernelCommandQueueDepth > gKernelCommandQueueHighWater) {
    gKernelCommandQueueHighWater = gKernelCommandQueueDepth;
  }
}

void enqueueKernelCommand(const KernelCommand& command) {
  if (gKernelCommandQueue == nullptr) return;
  if (xQueueSend(gKernelCommandQueue, &command, 0) != pdTRUE) {
    gKernelCommandQueueDropCount += 1;
  }
  updateKernelCommandQueueStats();
}

void applyKernelCommands(uint32_t nowMs) {
  if (gKernelCommandQueue == nullptr) return;

  KernelCommand command = {};
  while (xQueueReceive(gKernelCommandQueue, &command, 0) == pdTRUE) {
    gKernelCommandAppliedCount += 1;

    uint32_t latencyMs = 0;
    if (nowMs >= command.enqueuedAtMs) latencyMs = nowMs - command.enqueuedAtMs;
    gKernelCommandLastLatencyMs = latencyMs;
    if (latencyMs > gKernelCommandMaxLatencyMs) {
      gKernelCommandMaxLatencyMs = latencyMs;
    }

    switch (command.type) {
      case KernelCommandType::NoopHeartbeat:
        gKernelCommandNoopCount += 1;
        break;
      default:
        break;
    }
  }
  updateKernelCommandQueueStats();
}

KernelSnapshotMessage latestKernelSnapshotFromQueue() {
  KernelSnapshotMessage latest = {};
  latest.producedAtMs = gPlatform.nowMs();
  latest.metrics = gLastKernelMetrics;
  if (gKernelSnapshotQueue == nullptr) return latest;

  KernelSnapshotMessage next = {};
  while (xQueueReceive(gKernelSnapshotQueue, &next, 0) == pdTRUE) {
    latest = next;
  }
  gLastKernelMetrics = latest.metrics;
  updateKernelSnapshotQueueStats();
  return latest;
}

void core0KernelTask(void*) {
  while (true) {
    const uint32_t nowMs = gPlatform.nowMs();
    applyKernelCommands(nowMs);
    gKernel.tick(nowMs);
    KernelSnapshotMessage snapshot = {};
    snapshot.producedAtMs = nowMs;
    snapshot.metrics = gKernel.metrics();
    enqueueKernelSnapshot(snapshot);
    vTaskDelay(pdMS_TO_TICKS(kCore0LoopDelayMs));
  }
}

void core1ServiceTask(void*) {
  uint32_t commandSequence = 0;
  uint32_t lastHeartbeatMs = 0;

  while (true) {
    const uint32_t nowMs = gPlatform.nowMs();
    const KernelSnapshotMessage snapshot = latestKernelSnapshotFromQueue();

    gControl.tick(nowMs);
    gRuntime.tick(snapshot.producedAtMs, snapshot.metrics, gBootstrapDiagnostics);
    gPortal.tick(nowMs, gRuntime.snapshot());

    if ((nowMs - lastHeartbeatMs) >= kCommandHeartbeatIntervalMs) {
      KernelCommand heartbeat = {};
      heartbeat.type = KernelCommandType::NoopHeartbeat;
      heartbeat.sequence = ++commandSequence;
      heartbeat.enqueuedAtMs = nowMs;
      enqueueKernelCommand(heartbeat);
      lastHeartbeatMs = nowMs;
    }

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
  gLastKernelMetrics = gKernel.metrics();

  gRuntime.begin();
  gControl.begin();
  gPortal.begin();

  gKernelSnapshotQueue = xQueueCreate(kKernelSnapshotQueueCapacity,
                                      sizeof(KernelSnapshotMessage));
  if (gKernelSnapshotQueue == nullptr) {
    Serial.println("V3 snapshot queue bootstrap failed");
    while (true) {
      delay(1000);
    }
  }
  gKernelCommandQueue = xQueueCreate(kKernelCommandQueueCapacity, sizeof(KernelCommand));
  if (gKernelCommandQueue == nullptr) {
    Serial.println("V3 command queue bootstrap failed");
    while (true) {
      delay(1000);
    }
  }

  KernelSnapshotMessage initialSnapshot = {};
  initialSnapshot.producedAtMs = gPlatform.nowMs();
  initialSnapshot.metrics = gLastKernelMetrics;
  enqueueKernelSnapshot(initialSnapshot);

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
