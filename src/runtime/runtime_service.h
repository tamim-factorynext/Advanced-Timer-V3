/*
File: src/runtime/runtime_service.h
Purpose: Declares the runtime service module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/main.cpp
- src/portal/portal_service.h
Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/kernel_service.h"
#include "storage/storage_service.h"

namespace v3::runtime {

/**
 * @brief Consolidated queue and command-path telemetry snapshot.
 * @details Captures cross-core queue pressure and command lifecycle parity counters.
 * @par Used By
 * - src/main.cpp
 * - src/runtime/runtime_service.cpp
 * - src/portal/portal_service.cpp
 */
struct QueueTelemetry {
  uint16_t snapshotQueueDepth;
  uint16_t snapshotQueueHighWater;
  uint16_t snapshotQueueCapacity;
  uint32_t snapshotQueueDropCount;
  uint16_t commandQueueDepth;
  uint16_t commandQueueHighWater;
  uint16_t commandQueueCapacity;
  uint32_t commandQueueDropCount;
  uint32_t commandAppliedCount;
  uint32_t commandSetRunModeCount;
  uint32_t commandStepCount;
  uint8_t commandLastAppliedType;
  uint32_t commandLastLatencyUs;
  uint32_t commandMaxLatencyUs;
  uint32_t commandLastLatencyMs;
  uint32_t commandMaxLatencyMs;
  uint16_t controlPendingDepth;
  uint16_t controlPendingHighWater;
  uint32_t controlRequestedCount;
  uint32_t controlAcceptedCount;
  uint32_t controlRejectedCount;
  uint8_t controlLastRejectReason;
  uint32_t controlDispatchQueueFullCount;
  uint32_t portalRequestedCount;
  uint32_t portalAcceptedCount;
  uint32_t portalRejectedCount;
  uint8_t portalLastRejectReason;
  uint32_t portalLastRequestId;
  bool portalLastRequestAccepted;
  uint32_t portalQueueAcceptedCount;
  uint32_t portalQueueRejectedCount;
  uint32_t kernelLastAppliedRequestId;
  bool parityControlRequestedExceedsPortalAccepted;
  bool parityControlAcceptedExceedsControlRequested;
  bool parityKernelAppliedExceedsControlAccepted;
};

/**
 * @brief Runtime observability snapshot consumed by portal and diagnostics.
 * @details Projects kernel metrics, storage bootstrap status, and queue telemetry into one payload contract.
 * @par Used By
 * - src/runtime/runtime_service.cpp
 * - src/portal/portal_service.cpp
 */
struct RuntimeSnapshot {
  uint32_t nowMs;
  uint32_t completedScans;
  uint32_t lastScanMs;
  uint32_t scanLastUs;
  uint32_t scanMaxUs;
  uint32_t scanOverrunCount;
  bool scanOverrunLast;
  uint32_t scanPeriodMs;
  uint8_t configuredCardCount;
  uint8_t enabledCardCount;
  uint8_t diCardCount;
  uint8_t doCardCount;
  uint8_t aiCardCount;
  uint8_t sioCardCount;
  uint8_t mathCardCount;
  uint8_t rtcCardCount;
  uint8_t familyCountSum;
  bool bindingConsistent;
  engineMode mode;
  uint32_t stepAppliedCount;
  uint32_t diTotalQualifiedEdges;
  uint8_t diInhibitedCount;
  uint8_t diForcedCount;
  uint8_t aiForcedCount;
  uint8_t doActiveCount;
  uint8_t sioActiveCount;
  bool bootstrapUsedFileConfig;
  bool storageHasActiveConfig;
  v3::storage::ConfigErrorCode storageBootstrapError;
  QueueTelemetry queueTelemetry;
};

/**
 * @brief Owns construction of runtime snapshot contract from service inputs.
 * @details Stateless projection layer between kernel/storage/control telemetry and transport surfaces.
 * @par Used By
 * - src/main.cpp
 * - src/portal/portal_service.h
 */
class RuntimeService {
 public:
  /**
   * @brief Resets runtime snapshot state to defaults.
   * @details Called once at startup before runtime ticks begin.
   * @par Used By
   * - src/main.cpp
   */
  void begin();
  /**
   * @brief Updates runtime snapshot using latest kernel/storage/queue signals.
   * @details Should be called from service task after newest kernel snapshot is drained.
   * @param nowMs Current wall-clock milliseconds.
   * @param kernelMetrics Latest deterministic kernel metrics.
   * @param storageDiagnostics Storage bootstrap status snapshot.
   * @param queueTelemetry Current queue/command telemetry aggregate.
   * @par Used By
   * - src/main.cpp
   */
  void tick(uint32_t nowMs, const v3::kernel::KernelMetrics& kernelMetrics,
            const v3::storage::BootstrapDiagnostics& storageDiagnostics,
            const QueueTelemetry& queueTelemetry);
  /**
   * @brief Returns current runtime snapshot view.
   * @details Returned reference remains valid for service/portal read-only usage.
   * @return Immutable runtime snapshot.
   * @par Used By
   * - src/main.cpp
   * - src/portal/portal_service.cpp
   */
  const RuntimeSnapshot& snapshot() const;

 private:
  RuntimeSnapshot snapshot_ = {};
};

}  // namespace v3::runtime

