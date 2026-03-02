#include "kernel/kernel_service.h"

namespace v3::kernel {

void KernelService::begin(const v3::storage::ValidatedConfig& config) {
  config_ = config;
  metrics_.scanIntervalMs = config_.system.scanIntervalMs;
  metrics_.configuredCardCount = config_.system.cardCount;
  metrics_.completedScans = 0;
  metrics_.lastScanMs = 0;
  nextScanDueMs_ = 0;
}

void KernelService::tick(uint32_t nowMs) {
  if (metrics_.scanIntervalMs == 0) return;
  if (nowMs < nextScanDueMs_) return;

  metrics_.completedScans += 1;
  metrics_.lastScanMs = nowMs;
  nextScanDueMs_ = nowMs + metrics_.scanIntervalMs;
}

const KernelMetrics& KernelService::metrics() const { return metrics_; }

}  // namespace v3::kernel
