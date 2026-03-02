#include "runtime/runtime_service.h"

namespace v3::runtime {

void RuntimeService::begin() { snapshot_ = {}; }

void RuntimeService::tick(uint32_t nowMs,
                          const v3::kernel::KernelMetrics& kernelMetrics) {
  snapshot_.nowMs = nowMs;
  snapshot_.completedScans = kernelMetrics.completedScans;
  snapshot_.lastScanMs = kernelMetrics.lastScanMs;
  snapshot_.scanIntervalMs = kernelMetrics.scanIntervalMs;
}

const RuntimeSnapshot& RuntimeService::snapshot() const { return snapshot_; }

}  // namespace v3::runtime
