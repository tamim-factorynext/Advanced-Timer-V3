#pragma once

#include <stdint.h>

#include "storage/v3_config_validator.h"

namespace v3::kernel {

struct KernelMetrics {
  uint32_t scanIntervalMs;
  uint8_t configuredCardCount;
  uint32_t completedScans;
  uint32_t lastScanMs;
};

class KernelService {
 public:
  void begin(const v3::storage::ValidatedConfig& config);
  void tick(uint32_t nowMs);
  const KernelMetrics& metrics() const;

 private:
  v3::storage::ValidatedConfig config_ = {};
  KernelMetrics metrics_ = {};
  uint32_t nextScanDueMs_ = 0;
};

}  // namespace v3::kernel
