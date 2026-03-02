#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "control/command_dto.h"
#include "control/control_service.h"
#include "kernel/kernel_service.h"
#include "platform/platform_service.h"
#include "platform/wifi_runtime.h"
#include "portal/portal_service.h"
#include "portal/transport_runtime.h"
#include "runtime/runtime_snapshot_card.h"
#include "runtime/runtime_service.h"
#include "storage/storage_service.h"

namespace {

v3::platform::PlatformService gPlatform;
v3::platform::WiFiRuntime gWiFi;
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
uint32_t gKernelCommandSetRunModeCount = 0;
uint32_t gKernelCommandStepCount = 0;
uint32_t gKernelCommandLastLatencyMs = 0;
uint32_t gKernelCommandMaxLatencyMs = 0;
uint8_t gKernelCommandLastAppliedType = 0;
uint32_t gKernelLastAppliedRequestId = 0;
uint32_t gControlDispatchQueueFullCount = 0;
v3::platform::WiFiState gLastWiFiState = v3::platform::WiFiState::Offline;
uint32_t gLastWiFiLogMs = 0;

constexpr uint32_t kCore0LoopDelayMs = 1;
constexpr uint32_t kCore1LoopDelayMs = 1;
constexpr uint8_t kKernelSnapshotQueueCapacity = 8;
constexpr uint8_t kKernelCommandQueueCapacity = 16;
constexpr uint32_t kCommandHeartbeatIntervalMs = 250;
constexpr uint32_t kTaskWatchdogTimeoutSeconds = 8;
constexpr uint32_t kWiFiStatusLogIntervalMs = 10000;

struct KernelSnapshotMessage {
  uint32_t producedAtMs;
  v3::kernel::KernelMetrics metrics;
  uint8_t cardCount;
  RuntimeSnapshotCard cards[v3::storage::kMaxCards];
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

bool enqueueKernelCommand(const KernelCommand& command) {
  if (gKernelCommandQueue == nullptr) return false;
  if (xQueueSend(gKernelCommandQueue, &command, 0) != pdTRUE) {
    gKernelCommandQueueDropCount += 1;
    updateKernelCommandQueueStats();
    return false;
  }
  updateKernelCommandQueueStats();
  return true;
}

void applyKernelCommands(uint32_t nowUs) {
  if (gKernelCommandQueue == nullptr) return;

  ::KernelCommand command = {};
  while (xQueueReceive(gKernelCommandQueue, &command, 0) == pdTRUE) {
    gKernelCommandAppliedCount += 1;
    gKernelCommandLastAppliedType = static_cast<uint8_t>(command.type);

    uint32_t latencyMs = 0;
    if (nowUs >= command.enqueuedUs) {
      latencyMs = (nowUs - command.enqueuedUs) / 1000;
    }
    gKernelCommandLastLatencyMs = latencyMs;
    if (latencyMs > gKernelCommandMaxLatencyMs) {
      gKernelCommandMaxLatencyMs = latencyMs;
    }

    switch (command.type) {
      case KernelCmd_SetRunMode:
        gKernelCommandSetRunModeCount += 1;
        gKernelLastAppliedRequestId = command.requestId;
        gKernel.setRunMode(command.mode);
        break;
      case KernelCmd_StepOnce:
        gKernelCommandStepCount += 1;
        gKernelLastAppliedRequestId = command.requestId;
        gKernel.requestStepOnce();
        break;
      case KernelCmd_SetInputForce:
        gKernelLastAppliedRequestId = command.requestId;
        if (command.inputMode == InputSource_Real) {
          gKernel.setDiForce(command.cardId, false, false);
          gKernel.setAiForce(command.cardId, false, 0);
        } else if (command.inputMode == InputSource_ForcedHigh) {
          gKernel.setDiForce(command.cardId, true, true);
        } else if (command.inputMode == InputSource_ForcedLow) {
          gKernel.setDiForce(command.cardId, true, false);
        } else if (command.inputMode == InputSource_ForcedValue) {
          gKernel.setAiForce(command.cardId, true, command.value);
        }
        break;
      default:
        break;
    }
  }
  updateKernelCommandQueueStats();
}

void dispatchControlCommandsToKernelQueue() {
  ::KernelCommand command = {};
  while (gControl.dequeueCommand(command)) {
    if (!enqueueKernelCommand(command)) {
      gControlDispatchQueueFullCount += 1;
    }
  }
}

void dispatchPortalRequestsToControl() {
  v3::portal::PortalCommandRequest request = {};
  while (gPortal.dequeueCommandRequest(request)) {
    bool accepted = false;
    v3::control::CommandRejectReason rejectReason =
        v3::control::CommandRejectReason::None;

    switch (request.type) {
      case v3::portal::PortalCommandType::SetRunMode:
        accepted = gControl.requestSetRunMode(request.mode, request.enqueuedUs,
                                              request.requestId);
        break;
      case v3::portal::PortalCommandType::StepOnce:
        accepted = gControl.requestStepOnce(request.enqueuedUs, request.requestId);
        break;
      case v3::portal::PortalCommandType::SetInputForce:
        accepted = gControl.requestSetInputForce(request.cardId, request.inputMode,
                                                 request.enqueuedUs,
                                                 request.requestId,
                                                 request.inputValue);
        break;
      default:
        accepted = false;
        break;
    }

    if (!accepted) {
      rejectReason = gControl.diagnostics().lastRejectReason;
    }
    gPortal.recordCommandResult(request.requestId, accepted, rejectReason);
  }
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
  if (!gPlatform.addCurrentTaskToWatchdog()) {
    Serial.println("V3 watchdog add failed: core0");
    vTaskDelete(nullptr);
    return;
  }

  while (true) {
    const uint32_t nowUs = micros();
    const uint32_t nowMs = gPlatform.nowMs();
    applyKernelCommands(nowUs);
    gKernel.tick(nowMs);
    KernelSnapshotMessage snapshot = {};
    snapshot.producedAtMs = nowMs;
    snapshot.metrics = gKernel.metrics();
    snapshot.cardCount =
        gKernel.exportRuntimeSnapshotCards(snapshot.cards, v3::storage::kMaxCards);
    enqueueKernelSnapshot(snapshot);
    gPlatform.resetTaskWatchdog();
    vTaskDelay(pdMS_TO_TICKS(kCore0LoopDelayMs));
  }
}

void core1ServiceTask(void*) {
  if (!gPlatform.addCurrentTaskToWatchdog()) {
    Serial.println("V3 watchdog add failed: core1");
    vTaskDelete(nullptr);
    return;
  }

  uint32_t lastHeartbeatMs = 0;

  while (true) {
    const uint32_t nowMs = gPlatform.nowMs();
    gWiFi.tick(nowMs);
    const v3::platform::WiFiStatus& wifi = gWiFi.status();
    if (wifi.state != gLastWiFiState ||
        (nowMs - gLastWiFiLogMs) >= kWiFiStatusLogIntervalMs) {
      Serial.printf("V3 WiFi state=%u connected=%u retries=%lu ip=%s\n",
                    static_cast<unsigned>(wifi.state),
                    wifi.staConnected ? 1U : 0U,
                    static_cast<unsigned long>(wifi.retryCount), wifi.staIp);
      gLastWiFiState = wifi.state;
      gLastWiFiLogMs = nowMs;
    }

    const KernelSnapshotMessage snapshot = latestKernelSnapshotFromQueue();
    gControl.tick(nowMs);

    if ((nowMs - lastHeartbeatMs) >= kCommandHeartbeatIntervalMs) {
      gPortal.submitSetRunMode(RUN_NORMAL, micros());
      lastHeartbeatMs = nowMs;
    }

    dispatchPortalRequestsToControl();
    dispatchControlCommandsToKernelQueue();

    v3::runtime::QueueTelemetry queueTelemetry = {};
    queueTelemetry.snapshotQueueDepth = gKernelSnapshotQueueDepth;
    queueTelemetry.snapshotQueueHighWater = gKernelSnapshotQueueHighWater;
    queueTelemetry.snapshotQueueDropCount = gKernelSnapshotQueueDropCount;
    queueTelemetry.commandQueueDepth = gKernelCommandQueueDepth;
    queueTelemetry.commandQueueHighWater = gKernelCommandQueueHighWater;
    queueTelemetry.commandQueueDropCount = gKernelCommandQueueDropCount;
    queueTelemetry.commandAppliedCount = gKernelCommandAppliedCount;
    queueTelemetry.commandSetRunModeCount = gKernelCommandSetRunModeCount;
    queueTelemetry.commandStepCount = gKernelCommandStepCount;
    queueTelemetry.commandLastAppliedType = gKernelCommandLastAppliedType;
    queueTelemetry.commandLastLatencyMs = gKernelCommandLastLatencyMs;
    queueTelemetry.commandMaxLatencyMs = gKernelCommandMaxLatencyMs;
    const v3::control::ControlDiagnostics& controlDiag = gControl.diagnostics();
    queueTelemetry.controlPendingDepth = controlDiag.pendingDepth;
    queueTelemetry.controlPendingHighWater = controlDiag.pendingHighWater;
    queueTelemetry.controlRequestedCount = controlDiag.requestedCount;
    queueTelemetry.controlAcceptedCount = controlDiag.acceptedCount;
    queueTelemetry.controlRejectedCount = controlDiag.rejectedCount;
    queueTelemetry.controlLastRejectReason =
        static_cast<uint8_t>(controlDiag.lastRejectReason);
    queueTelemetry.controlDispatchQueueFullCount =
        gControlDispatchQueueFullCount;
    const v3::portal::PortalCommandIngressDiagnostics& portalDiag =
        gPortal.commandIngressDiagnostics();
    queueTelemetry.portalRequestedCount = portalDiag.requestedCount;
    queueTelemetry.portalAcceptedCount = portalDiag.acceptedCount;
    queueTelemetry.portalRejectedCount = portalDiag.rejectedCount;
    queueTelemetry.portalLastRejectReason = portalDiag.lastRejectReason;
    queueTelemetry.portalLastRequestId = portalDiag.lastRequestId;
    queueTelemetry.portalLastRequestAccepted = portalDiag.lastRequestAccepted;
    queueTelemetry.portalQueueAcceptedCount = portalDiag.queueAcceptedCount;
    queueTelemetry.portalQueueRejectedCount = portalDiag.queueRejectedCount;
    queueTelemetry.kernelLastAppliedRequestId = gKernelLastAppliedRequestId;
    queueTelemetry.parityControlRequestedExceedsPortalAccepted =
        (queueTelemetry.controlRequestedCount > queueTelemetry.portalAcceptedCount);
    queueTelemetry.parityControlAcceptedExceedsControlRequested =
        (queueTelemetry.controlAcceptedCount >
         queueTelemetry.controlRequestedCount);
    queueTelemetry.parityKernelAppliedExceedsControlAccepted =
        (queueTelemetry.commandAppliedCount >
         queueTelemetry.controlAcceptedCount);

    gRuntime.tick(snapshot.producedAtMs, snapshot.metrics, gBootstrapDiagnostics,
                  queueTelemetry);
    gPortal.tick(nowMs, gRuntime.snapshot(), snapshot.cards, snapshot.cardCount);
    v3::portal::serviceTransportRuntime();

    gPlatform.resetTaskWatchdog();
    vTaskDelay(pdMS_TO_TICKS(kCore1LoopDelayMs));
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Advanced Timer V3 Core bootstrap");

  gPlatform.begin();
  if (!gPlatform.initTaskWatchdog(kTaskWatchdogTimeoutSeconds, true)) {
    Serial.println("V3 watchdog init failed");
    while (true) {
      delay(1000);
    }
  }
  gStorage.begin();

  if (!gStorage.hasActiveConfig()) {
    const v3::storage::ConfigValidationError err = gStorage.lastError();
    Serial.print("V3 config bootstrap failed: ");
    Serial.println(v3::storage::configErrorCodeToString(err.code));
    while (true) {
      delay(1000);
    }
  }

  gKernel.begin(gStorage.activeConfig(), gPlatform);
  gBootstrapDiagnostics = gStorage.diagnostics();
  gLastKernelMetrics = gKernel.metrics();

  gRuntime.begin();
  gControl.begin();
  gPortal.begin();
  gWiFi.begin(gStorage.activeConfig().system.wifi);
  v3::portal::initTransportRuntime(gPortal);

  gKernelSnapshotQueue = xQueueCreate(kKernelSnapshotQueueCapacity,
                                      sizeof(KernelSnapshotMessage));
  if (gKernelSnapshotQueue == nullptr) {
    Serial.println("V3 snapshot queue bootstrap failed");
    while (true) {
      delay(1000);
    }
  }
  gKernelCommandQueue =
      xQueueCreate(kKernelCommandQueueCapacity, sizeof(::KernelCommand));
  if (gKernelCommandQueue == nullptr) {
    Serial.println("V3 command queue bootstrap failed");
    while (true) {
      delay(1000);
    }
  }

  KernelSnapshotMessage initialSnapshot = {};
  initialSnapshot.producedAtMs = gPlatform.nowMs();
  initialSnapshot.metrics = gLastKernelMetrics;
  initialSnapshot.cardCount = gKernel.exportRuntimeSnapshotCards(
      initialSnapshot.cards, v3::storage::kMaxCards);
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
