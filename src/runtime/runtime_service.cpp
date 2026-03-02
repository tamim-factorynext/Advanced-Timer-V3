#include "runtime/runtime_service.h"

namespace v3::runtime {

void RuntimeService::begin() { snapshot_ = {}; }

void RuntimeService::tick(uint32_t nowMs,
                          const v3::kernel::KernelMetrics& kernelMetrics,
                          const v3::storage::BootstrapDiagnostics&
                              storageDiagnostics) {
  snapshot_.nowMs = nowMs;
  snapshot_.completedScans = kernelMetrics.completedScans;
  snapshot_.lastScanMs = kernelMetrics.lastScanMs;
  snapshot_.scanIntervalMs = kernelMetrics.scanIntervalMs;
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
  snapshot_.bootstrapUsedFileConfig =
      (storageDiagnostics.source == v3::storage::BootstrapSource::FileConfig);
  snapshot_.storageHasActiveConfig = storageDiagnostics.hasActiveConfig;
  snapshot_.storageBootstrapError = storageDiagnostics.error.code;
}

const RuntimeSnapshot& RuntimeService::snapshot() const { return snapshot_; }

}  // namespace v3::runtime
