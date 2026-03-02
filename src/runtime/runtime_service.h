#pragma once

#include <stdint.h>

#include "kernel/kernel_service.h"
#include "storage/storage_service.h"

namespace v3::runtime {

struct QueueTelemetry {
  uint16_t snapshotQueueDepth;
  uint16_t snapshotQueueHighWater;
  uint32_t snapshotQueueDropCount;
  uint16_t commandQueueDepth;
  uint16_t commandQueueHighWater;
  uint32_t commandQueueDropCount;
  uint32_t commandAppliedCount;
  uint32_t commandSetRunModeCount;
  uint32_t commandStepCount;
  uint8_t commandLastAppliedType;
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

struct RuntimeSnapshot {
  uint32_t nowMs;
  uint32_t completedScans;
  uint32_t lastScanMs;
  uint32_t scanIntervalMs;
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
  runMode mode;
  uint32_t stepAppliedCount;
  uint32_t diTotalQualifiedEdges;
  uint8_t diInhibitedCount;
  uint8_t diForcedCount;
  uint8_t aiForcedCount;
  bool bootstrapUsedFileConfig;
  bool storageHasActiveConfig;
  v3::storage::ConfigErrorCode storageBootstrapError;
  QueueTelemetry queueTelemetry;
};

class RuntimeService {
 public:
  void begin();
  void tick(uint32_t nowMs, const v3::kernel::KernelMetrics& kernelMetrics,
            const v3::storage::BootstrapDiagnostics& storageDiagnostics,
            const QueueTelemetry& queueTelemetry);
  const RuntimeSnapshot& snapshot() const;

 private:
  RuntimeSnapshot snapshot_ = {};
};

}  // namespace v3::runtime
