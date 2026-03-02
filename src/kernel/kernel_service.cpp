#include "kernel/kernel_service.h"

namespace v3::kernel {

namespace {

void resetFamilyCounts(KernelMetrics& metrics) {
  metrics.enabledCardCount = 0;
  metrics.diCardCount = 0;
  metrics.doCardCount = 0;
  metrics.aiCardCount = 0;
  metrics.sioCardCount = 0;
  metrics.mathCardCount = 0;
  metrics.rtcCardCount = 0;
  metrics.familyCountSum = 0;
  metrics.bindingConsistent = false;
}

void bindTypedConfigSummary(const v3::storage::ValidatedConfig& config,
                            KernelMetrics& metrics) {
  resetFamilyCounts(metrics);
  for (uint8_t i = 0; i < config.system.cardCount; ++i) {
    const v3::storage::CardConfig& card = config.system.cards[i];
    if (card.enabled) metrics.enabledCardCount += 1;

    switch (card.family) {
      case v3::storage::CardFamily::DI:
        metrics.diCardCount += 1;
        break;
      case v3::storage::CardFamily::DO:
        metrics.doCardCount += 1;
        break;
      case v3::storage::CardFamily::AI:
        metrics.aiCardCount += 1;
        break;
      case v3::storage::CardFamily::SIO:
        metrics.sioCardCount += 1;
        break;
      case v3::storage::CardFamily::MATH:
        metrics.mathCardCount += 1;
        break;
      case v3::storage::CardFamily::RTC:
        metrics.rtcCardCount += 1;
        break;
      default:
        break;
    }
  }

  metrics.familyCountSum = static_cast<uint8_t>(
      metrics.diCardCount + metrics.doCardCount + metrics.aiCardCount +
      metrics.sioCardCount + metrics.mathCardCount + metrics.rtcCardCount);
  metrics.bindingConsistent =
      (metrics.familyCountSum == metrics.configuredCardCount) &&
      (metrics.enabledCardCount <= metrics.configuredCardCount);
}

}  // namespace

void KernelService::begin(const v3::storage::ValidatedConfig& config) {
  config_ = config;
  metrics_.scanIntervalMs = config_.system.scanIntervalMs;
  metrics_.configuredCardCount = config_.system.cardCount;
  bindTypedConfigSummary(config_, metrics_);
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
