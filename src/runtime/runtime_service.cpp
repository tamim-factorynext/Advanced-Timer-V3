/*
File: src/runtime/runtime_service.cpp
Purpose: Implements the runtime service module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/main.cpp
- src/portal/portal_service.cpp
Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "runtime/runtime_service.h"

namespace v3::runtime {

/**
 * @brief Resets runtime snapshot contract to default state.
 * @par Used By
 * Main startup initialization.
 */
void RuntimeService::begin() { snapshot_ = {}; }

/**
 * @brief Refreshes runtime snapshot from latest kernel/storage/control state.
 * @param nowMs Current monotonic time in milliseconds.
 * @param kernelMetrics Kernel execution and family counters.
 * @param storageDiagnostics Storage bootstrap and config-health status.
 * @param queueTelemetry Command-path queue telemetry.
 * @par Used By
 * Main loop runtime observability refresh.
 */
void RuntimeService::tick(uint32_t nowMs,
                          const v3::kernel::KernelMetrics& kernelMetrics,
                          const v3::storage::BootstrapDiagnostics&
                              storageDiagnostics,
                          const QueueTelemetry& queueTelemetry) {
  snapshot_.nowMs = nowMs;
  snapshot_.completedScans = kernelMetrics.completedScans;
  snapshot_.lastScanMs = kernelMetrics.lastScanMs;
  snapshot_.scanLastUs = kernelMetrics.scanLastUs;
  snapshot_.scanMaxUs = kernelMetrics.scanMaxUs;
  snapshot_.scanOverrunCount = kernelMetrics.scanOverrunCount;
  snapshot_.scanOverrunLast = kernelMetrics.scanOverrunLast;
  snapshot_.scanPeriodMs = kernelMetrics.scanPeriodMs;
  snapshot_.configuredCardCount = kernelMetrics.configuredCardCount;
  snapshot_.enabledCardCount = kernelMetrics.enabledCardCount;
  snapshot_.diCardCount = kernelMetrics.diCardCount;
  snapshot_.doCardCount = kernelMetrics.doCardCount;
  snapshot_.aiCardCount = kernelMetrics.aiCardCount;
  snapshot_.sioCardCount = kernelMetrics.sioCardCount;
  snapshot_.mathCardCount = kernelMetrics.mathCardCount;
  snapshot_.rtcCardCount = kernelMetrics.rtcCardCount;
  snapshot_.familyCountSum = kernelMetrics.familyCountSum;
  snapshot_.bindingConsistent = kernelMetrics.bindingConsistent;
  snapshot_.mode = kernelMetrics.mode;
  snapshot_.stepAppliedCount = kernelMetrics.stepAppliedCount;
  snapshot_.diTotalQualifiedEdges = kernelMetrics.diTotalQualifiedEdges;
  snapshot_.diInhibitedCount = kernelMetrics.diInhibitedCount;
  snapshot_.diForcedCount = kernelMetrics.diForcedCount;
  snapshot_.aiForcedCount = kernelMetrics.aiForcedCount;
  snapshot_.doActiveCount = kernelMetrics.doActiveCount;
  snapshot_.sioActiveCount = kernelMetrics.sioActiveCount;
  snapshot_.bootstrapUsedFileConfig =
      (storageDiagnostics.source == v3::storage::BootstrapSource::FileConfig);
  snapshot_.storageHasActiveConfig = storageDiagnostics.hasActiveConfig;
  snapshot_.storageBootstrapError = storageDiagnostics.error.code;
  snapshot_.queueTelemetry = queueTelemetry;
}

/**
 * @brief Returns current runtime snapshot.
 * @return Immutable runtime snapshot reference.
 * @par Used By
 * Portal JSON cache builders and transport surfaces.
 */
const RuntimeSnapshot& RuntimeService::snapshot() const { return snapshot_; }

}  // namespace v3::runtime

