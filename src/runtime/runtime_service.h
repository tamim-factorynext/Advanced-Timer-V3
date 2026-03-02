#pragma once

#include <stdint.h>

#include "kernel/kernel_service.h"

namespace v3::runtime {

struct RuntimeSnapshot {
  uint32_t nowMs;
  uint32_t completedScans;
  uint32_t lastScanMs;
  uint32_t scanIntervalMs;
};

class RuntimeService {
 public:
  void begin();
  void tick(uint32_t nowMs, const v3::kernel::KernelMetrics& kernelMetrics);
  const RuntimeSnapshot& snapshot() const;

 private:
  RuntimeSnapshot snapshot_ = {};
};

}  // namespace v3::runtime
