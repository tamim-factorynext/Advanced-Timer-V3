/*
File: src/main.cpp
Purpose: Bootstraps services and starts the firmware runtime loop.

Responsibilities:
- Initialize services, queues, and task orchestration.
- Wire module boundaries for runtime execution and transport.

Used By:
- firmware build target
Flow Hook:
- Firmware entrypoint and service wiring.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
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
uint32_t gKernelCommandLastLatencyUs = 0;
uint32_t gKernelCommandMaxLatencyUs = 0;
uint32_t gKernelCommandLastLatencyMs = 0;
uint32_t gKernelCommandMaxLatencyMs = 0;
uint8_t gKernelCommandLastAppliedType = 0;
uint32_t gKernelLastAppliedRequestId = 0;
uint32_t gControlDispatchQueueFullCount = 0;
v3::platform::WiFiState gLastWiFiState = v3::platform::WiFiState::Offline;
uint32_t gLastWiFiLogMs = 0;
uint32_t gMinFreeHeapBytes = 0xFFFFFFFFUL;
uint32_t gMinLargestFreeBlockBytes = 0xFFFFFFFFUL;
uint32_t gCore0StackHighWaterBytes = 0;
uint32_t gCore1StackHighWaterBytes = 0;
uint32_t gCore1LastStackLogMs = 0;
uint32_t gCore1StageMinLoopStart = 0xFFFFFFFFUL;
uint32_t gCore1StageMinAfterWifi = 0xFFFFFFFFUL;
uint32_t gCore1StageMinAfterSnapshotQueue = 0xFFFFFFFFUL;
uint32_t gCore1StageMinAfterControl = 0xFFFFFFFFUL;
uint32_t gCore1StageMinAfterDispatch = 0xFFFFFFFFUL;
uint32_t gCore1StageMinAfterRuntime = 0xFFFFFFFFUL;
uint32_t gCore1StageMinAfterPortalTick = 0xFFFFFFFFUL;
uint32_t gCore1StageMinAfterTransport = 0xFFFFFFFFUL;

constexpr uint32_t kCore0LoopDelayMs = 1;
constexpr uint32_t kCore1LoopDelayMs = 1;
constexpr uint32_t kCore0TaskStackBytes = 14336;
constexpr uint32_t kCore1TaskStackBytes = 32768;
constexpr uint8_t kKernelSnapshotQueueCapacity = 8;
constexpr uint8_t kKernelCommandQueueCapacity = 16;
constexpr uint32_t kTaskWatchdogTimeoutSeconds = 16;
constexpr uint32_t kWiFiStatusLogIntervalMs = 10000;
constexpr uint32_t kPortalProjectionActiveIntervalMs = 10;
constexpr uint32_t kPortalProjectionIdleIntervalMs = 500;
constexpr uint32_t kTransportActivityWindowMs = 15000;
constexpr uint32_t kCore1IdleLoopDelayMs = 5;
constexpr uint32_t kCore1StackStageLogIntervalMs = 5000;
constexpr bool kLogSetupStages = false;
constexpr bool kLogCoreTaskStartup = false;
constexpr bool kLogCore1StageSummary = false;
constexpr bool kLogWiFiHeartbeat = false;
constexpr bool kLogBootSummary = true;

void feedBootWatchdog() {
  const esp_err_t err = esp_task_wdt_reset();
  (void)err;
}

void updateTaskStackHighWaterBytes(uint32_t& outBytes) {
  const UBaseType_t words = uxTaskGetStackHighWaterMark(nullptr);
  const uint32_t bytes = static_cast<uint32_t>(words) * sizeof(StackType_t);
  if (outBytes == 0 || bytes < outBytes) outBytes = bytes;
}

uint32_t core1CurrentFreeStackBytes() {
  const UBaseType_t words = uxTaskGetStackHighWaterMark(nullptr);
  return static_cast<uint32_t>(words) * sizeof(StackType_t);
}

void updateCore1StageMin(uint32_t& stageMin) {
  const uint32_t freeBytes = core1CurrentFreeStackBytes();
  if (freeBytes < stageMin) stageMin = freeBytes;
}

void logCore1StageSummary(uint32_t nowMs) {
  if (!kLogCore1StageSummary) return;
  if ((nowMs - gCore1LastStackLogMs) < kCore1StackStageLogIntervalMs) return;
  Serial.printf(
      "[core1:stage-min] loop=%lu wifi=%lu snapQ=%lu ctrl=%lu dispatch=%lu "
      "runtime=%lu portal=%lu transport=%lu globalMin=%lu\n",
      static_cast<unsigned long>(gCore1StageMinLoopStart),
      static_cast<unsigned long>(gCore1StageMinAfterWifi),
      static_cast<unsigned long>(gCore1StageMinAfterSnapshotQueue),
      static_cast<unsigned long>(gCore1StageMinAfterControl),
      static_cast<unsigned long>(gCore1StageMinAfterDispatch),
      static_cast<unsigned long>(gCore1StageMinAfterRuntime),
      static_cast<unsigned long>(gCore1StageMinAfterPortalTick),
      static_cast<unsigned long>(gCore1StageMinAfterTransport),
      static_cast<unsigned long>(gCore1StackHighWaterBytes));
  Serial.flush();
  gCore1LastStackLogMs = nowMs;
}

[[noreturn]] void haltBoot(const char* message) {
  if (message != nullptr) {
    Serial.println(message);
    Serial.flush();
  }
  while (true) {
    feedBootWatchdog();
    delay(250);
  }
}

struct KernelSnapshotMessage {
  uint32_t producedAtMs;
  v3::kernel::KernelMetrics metrics;
  uint8_t cardCount;
  RuntimeSnapshotCard cards[v3::storage::kMaxCards];
};
KernelSnapshotMessage gCore0SnapshotBuffer = {};
KernelSnapshotMessage gCore1SnapshotBuffer = {};

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

    uint32_t latencyUs = 0;
    if (nowUs >= command.enqueuedUs) {
      latencyUs = (nowUs - command.enqueuedUs);
    }
    const uint32_t latencyMs = latencyUs / 1000U;
    gKernelCommandLastLatencyUs = latencyUs;
    if (latencyUs > gKernelCommandMaxLatencyUs) {
      gKernelCommandMaxLatencyUs = latencyUs;
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

void latestKernelSnapshotFromQueue(KernelSnapshotMessage& latest) {
  latest.producedAtMs = gPlatform.nowMs();
  latest.metrics = gLastKernelMetrics;
  if (gKernelSnapshotQueue == nullptr) return;

  while (xQueueReceive(gKernelSnapshotQueue, &latest, 0) == pdTRUE) {
  }
  gLastKernelMetrics = latest.metrics;
  updateKernelSnapshotQueueStats();
}

void core0KernelTask(void*) {
  if (kLogCoreTaskStartup) {
    Serial.println("[core0] 01 task entry");
    Serial.flush();
  }
  if (!gPlatform.addCurrentTaskToWatchdog()) {
    Serial.println("V3 watchdog add failed: core0");
    vTaskDelete(nullptr);
    return;
  }
  if (kLogCoreTaskStartup) {
    Serial.println("[core0] 02 watchdog registered");
    Serial.flush();
  }

  bool firstLoopLog = true;
  uint32_t lastPublishedScanCount = 0;
  bool snapshotPublishInitialized = false;

  while (true) {
    if (firstLoopLog) {
      if (kLogCoreTaskStartup) {
        Serial.println("[core0] 03 first loop tick");
        const UBaseType_t hw = uxTaskGetStackHighWaterMark(nullptr);
        Serial.printf("[core0] stack high-water words=%lu bytes=%lu\n",
                      static_cast<unsigned long>(hw),
                      static_cast<unsigned long>(hw * sizeof(StackType_t)));
        Serial.flush();
      }
      firstLoopLog = false;
    }
    const uint32_t nowUs = micros();
    const uint32_t nowMs = gPlatform.nowMs();
    updateTaskStackHighWaterBytes(gCore0StackHighWaterBytes);
    applyKernelCommands(nowUs);
    gKernel.tick(nowMs);
    gCore0SnapshotBuffer.metrics = gKernel.metrics();
    const uint32_t currentScanCount = gCore0SnapshotBuffer.metrics.completedScans;
    const bool hasNewScan = !snapshotPublishInitialized ||
                            (currentScanCount != lastPublishedScanCount);
    if (hasNewScan) {
      gCore0SnapshotBuffer.producedAtMs = nowMs;
      gCore0SnapshotBuffer.cardCount = gKernel.exportRuntimeSnapshotCards(
          gCore0SnapshotBuffer.cards, v3::storage::kMaxCards);
      enqueueKernelSnapshot(gCore0SnapshotBuffer);
      lastPublishedScanCount = currentScanCount;
      snapshotPublishInitialized = true;
    }
    gPlatform.resetTaskWatchdog();
    vTaskDelay(pdMS_TO_TICKS(kCore0LoopDelayMs));
  }
}

void core1ServiceTask(void*) {
  if (kLogCoreTaskStartup) {
    Serial.println("[core1] 01 task entry");
    Serial.flush();
  }
  if (!gPlatform.addCurrentTaskToWatchdog()) {
    Serial.println("V3 watchdog add failed: core1");
    vTaskDelete(nullptr);
    return;
  }
  if (kLogCoreTaskStartup) {
    Serial.println("[core1] 02 watchdog registered");
    Serial.flush();
  }

  bool firstLoopLog = true;
  uint32_t lastProjectedScanCount = 0;
  uint32_t lastPortalProjectionMs = 0;
  bool projectionInitialized = false;

  while (true) {
    if (firstLoopLog) {
      if (kLogCoreTaskStartup) {
        Serial.println("[core1] 03 first loop tick");
        const UBaseType_t hw = uxTaskGetStackHighWaterMark(nullptr);
        Serial.printf("[core1] stack high-water words=%lu bytes=%lu\n",
                      static_cast<unsigned long>(hw),
                      static_cast<unsigned long>(hw * sizeof(StackType_t)));
        Serial.flush();
      }
      firstLoopLog = false;
    }
    const uint32_t nowMs = gPlatform.nowMs();
    updateTaskStackHighWaterBytes(gCore1StackHighWaterBytes);
    updateCore1StageMin(gCore1StageMinLoopStart);
    const uint32_t freeHeapBytes = ESP.getFreeHeap();
    if (freeHeapBytes < gMinFreeHeapBytes) gMinFreeHeapBytes = freeHeapBytes;
    const uint32_t largestFreeBlockBytes =
        static_cast<uint32_t>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    if (largestFreeBlockBytes < gMinLargestFreeBlockBytes) {
      gMinLargestFreeBlockBytes = largestFreeBlockBytes;
    }
    const bool transportActive = v3::portal::hasRecentTransportActivity(
        nowMs, kTransportActivityWindowMs);
    gWiFi.tick(nowMs);
    updateCore1StageMin(gCore1StageMinAfterWifi);
    const v3::platform::WiFiStatus& wifi = gWiFi.status();
    if (wifi.state != gLastWiFiState ||
        (kLogWiFiHeartbeat &&
         (nowMs - gLastWiFiLogMs) >= kWiFiStatusLogIntervalMs)) {
      Serial.printf("V3 WiFi state=%u connected=%u retries=%lu ip=%s\n",
                    static_cast<unsigned>(wifi.state),
                    wifi.staConnected ? 1U : 0U,
                    static_cast<unsigned long>(wifi.retryCount), wifi.staIp);
      gLastWiFiState = wifi.state;
      gLastWiFiLogMs = nowMs;
    }

    latestKernelSnapshotFromQueue(gCore1SnapshotBuffer);
    updateCore1StageMin(gCore1StageMinAfterSnapshotQueue);
    gControl.tick(nowMs);
    updateCore1StageMin(gCore1StageMinAfterControl);

    dispatchPortalRequestsToControl();
    dispatchControlCommandsToKernelQueue();
    updateCore1StageMin(gCore1StageMinAfterDispatch);

    v3::runtime::QueueTelemetry queueTelemetry = {};
    queueTelemetry.snapshotQueueDepth = gKernelSnapshotQueueDepth;
    queueTelemetry.snapshotQueueHighWater = gKernelSnapshotQueueHighWater;
    queueTelemetry.snapshotQueueCapacity = kKernelSnapshotQueueCapacity;
    queueTelemetry.snapshotQueueDropCount = gKernelSnapshotQueueDropCount;
    queueTelemetry.commandQueueDepth = gKernelCommandQueueDepth;
    queueTelemetry.commandQueueHighWater = gKernelCommandQueueHighWater;
    queueTelemetry.commandQueueCapacity = kKernelCommandQueueCapacity;
    queueTelemetry.commandQueueDropCount = gKernelCommandQueueDropCount;
    queueTelemetry.commandAppliedCount = gKernelCommandAppliedCount;
    queueTelemetry.commandSetRunModeCount = gKernelCommandSetRunModeCount;
    queueTelemetry.commandStepCount = gKernelCommandStepCount;
    queueTelemetry.commandLastAppliedType = gKernelCommandLastAppliedType;
    queueTelemetry.commandLastLatencyUs = gKernelCommandLastLatencyUs;
    queueTelemetry.commandMaxLatencyUs = gKernelCommandMaxLatencyUs;
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

    v3::runtime::MemoryTelemetry memoryTelemetry = {};
    memoryTelemetry.minFreeHeapBytes = gMinFreeHeapBytes;
    memoryTelemetry.minLargestFreeBlockBytes = gMinLargestFreeBlockBytes;
    memoryTelemetry.core0StackHighWaterBytes = gCore0StackHighWaterBytes;
    memoryTelemetry.core1StackHighWaterBytes = gCore1StackHighWaterBytes;
    gRuntime.tick(gCore1SnapshotBuffer.producedAtMs, gCore1SnapshotBuffer.metrics,
                  gBootstrapDiagnostics, queueTelemetry, memoryTelemetry);
    updateCore1StageMin(gCore1StageMinAfterRuntime);
    const uint32_t currentScanCount =
        gCore1SnapshotBuffer.metrics.completedScans;
    const bool hasNewScan =
        !projectionInitialized || (currentScanCount != lastProjectedScanCount);
    const uint32_t projectionIntervalMs = transportActive
                                              ? kPortalProjectionActiveIntervalMs
                                              : kPortalProjectionIdleIntervalMs;
    const bool intervalElapsed =
        !projectionInitialized ||
        ((nowMs - lastPortalProjectionMs) >= projectionIntervalMs);
    const bool shouldProject = transportActive ? (hasNewScan || intervalElapsed)
                                               : intervalElapsed;
    if (shouldProject) {
      gPortal.tick(nowMs, gRuntime.snapshot(), gCore1SnapshotBuffer.cards,
                   gCore1SnapshotBuffer.cardCount);
      updateCore1StageMin(gCore1StageMinAfterPortalTick);
      lastProjectedScanCount = currentScanCount;
      lastPortalProjectionMs = nowMs;
      projectionInitialized = true;
    }
    v3::portal::serviceTransportRuntime();
    updateCore1StageMin(gCore1StageMinAfterTransport);
    logCore1StageSummary(nowMs);

    gPlatform.resetTaskWatchdog();
    const uint32_t core1DelayMs =
        transportActive ? kCore1LoopDelayMs : kCore1IdleLoopDelayMs;
    vTaskDelay(pdMS_TO_TICKS(core1DelayMs));
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);  // Keep startup capture window without starving loop-task WDT.
  if (kLogBootSummary) {
    Serial.println("Advanced Timer V3 Core bootstrap");
    Serial.printf("[boot] reset reason=%d\n",
                  static_cast<int>(esp_reset_reason()));
    Serial.flush();
  }
  feedBootWatchdog();

  auto logSetupStage = [](const char *stage) {
    if (!kLogSetupStages) {
      feedBootWatchdog();
      return;
    }
    Serial.print("[setup] ");
    Serial.println(stage);
    Serial.flush();
    feedBootWatchdog();
  };

  logSetupStage("01 begin platform");
  gPlatform.begin();
  logSetupStage("02 platform begin done");
  const v3::platform::HardwareProfile& hwProfile = gPlatform.profile();
  if (kLogBootSummary) {
    Serial.printf(
        "V3 hardware profile=%s variant=%s di=%u do=%u ai=%u sio=%u math=%u rtc=%u"
        " backends(di/do/ai/rtc)=%u/%u/%u/%u\n",
        hwProfile.profileName, hwProfile.platformVariant,
        static_cast<unsigned>(hwProfile.diCount),
        static_cast<unsigned>(hwProfile.doCount),
        static_cast<unsigned>(hwProfile.aiCount),
        static_cast<unsigned>(hwProfile.sioCapacity),
        static_cast<unsigned>(hwProfile.mathCapacity),
        static_cast<unsigned>(hwProfile.rtcCapacity),
        static_cast<unsigned>(hwProfile.diBackend),
        static_cast<unsigned>(hwProfile.doBackend),
        static_cast<unsigned>(hwProfile.aiBackend),
        static_cast<unsigned>(hwProfile.rtcBackend));
    Serial.flush();
  }
  logSetupStage("03 profile print done");

  logSetupStage("04 storage begin");
  gStorage.begin();
  logSetupStage("05 storage begin done");

  logSetupStage("06 hasActiveConfig check");
  if (!gStorage.hasActiveConfig()) {
    const v3::storage::ConfigValidationError err = gStorage.lastError();
    Serial.print("V3 config bootstrap failed: ");
    Serial.println(v3::storage::configErrorCodeToString(err.code));
    haltBoot("V3 halting: storage has no active config");
  }
  logSetupStage("07 active config available");

  logSetupStage("08 kernel begin");
  gKernel.begin(gStorage.activeConfig(), gPlatform);
  logSetupStage("09 kernel begin done");
  gBootstrapDiagnostics = gStorage.diagnostics();
  gLastKernelMetrics = gKernel.metrics();
  logSetupStage("10 diagnostics captured");

  logSetupStage("11 runtime begin");
  gRuntime.begin();
  logSetupStage("12 runtime begin done");
  logSetupStage("13 control begin");
  gControl.begin();
  logSetupStage("14 control begin done");
  logSetupStage("15 portal begin");
  gPortal.begin();
  logSetupStage("16 portal begin done");
  logSetupStage("17 wifi begin");
  gWiFi.begin(gStorage.activeConfig().system.wifi,
              gStorage.activeConfig().system.time);
  logSetupStage("18 wifi begin done");
  logSetupStage("19 transport init");
  v3::portal::initTransportRuntime(gPortal, gStorage);
  logSetupStage("20 transport init done");

  // Initialize watchdog after heavier bootstrap work so first-boot setup
  // does not get reset before runtime tasks start feeding the watchdog.
  logSetupStage("21 watchdog init");
  if (!gPlatform.initTaskWatchdog(kTaskWatchdogTimeoutSeconds, true)) {
    haltBoot("V3 watchdog init failed");
  }
  logSetupStage("22 watchdog init done");

  logSetupStage("23 create snapshot queue");
  gKernelSnapshotQueue = xQueueCreate(kKernelSnapshotQueueCapacity,
                                      sizeof(KernelSnapshotMessage));
  if (gKernelSnapshotQueue == nullptr) {
    haltBoot("V3 snapshot queue bootstrap failed");
  }
  logSetupStage("24 snapshot queue ready");
  logSetupStage("25 create command queue");
  gKernelCommandQueue =
      xQueueCreate(kKernelCommandQueueCapacity, sizeof(::KernelCommand));
  if (gKernelCommandQueue == nullptr) {
    haltBoot("V3 command queue bootstrap failed");
  }
  logSetupStage("26 command queue ready");

  logSetupStage("27 enqueue initial snapshot");
  gCore0SnapshotBuffer.producedAtMs = gPlatform.nowMs();
  gCore0SnapshotBuffer.metrics = gLastKernelMetrics;
  gCore0SnapshotBuffer.cardCount = gKernel.exportRuntimeSnapshotCards(
      gCore0SnapshotBuffer.cards, v3::storage::kMaxCards);
  enqueueKernelSnapshot(gCore0SnapshotBuffer);
  logSetupStage("28 initial snapshot queued");

  logSetupStage("29 create core0/core1 tasks");
  BaseType_t core0Created = xTaskCreatePinnedToCore(
      core0KernelTask, "v3-core0-kernel", kCore0TaskStackBytes, nullptr, 2,
      &gCore0TaskHandle, 0);
  BaseType_t core1Created = xTaskCreatePinnedToCore(
      core1ServiceTask, "v3-core1-services", kCore1TaskStackBytes, nullptr, 1,
      &gCore1TaskHandle, 1);

  if (core0Created != pdPASS || core1Created != pdPASS) {
    haltBoot("V3 dual-core task bootstrap failed");
  }
  logSetupStage("30 setup complete");
  if (kLogBootSummary) {
    Serial.println("V3 boot complete");
    Serial.flush();
  }
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
