#pragma once

#include <stdint.h>

#include "kernel/kernel_service.h"
#include "storage/storage_service.h"

namespace v3::runtime {

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
  bool bootstrapUsedFileConfig;
  bool storageHasActiveConfig;
  v3::storage::ConfigErrorCode storageBootstrapError;
};

class RuntimeService {
 public:
  void begin();
  void tick(uint32_t nowMs, const v3::kernel::KernelMetrics& kernelMetrics,
            const v3::storage::BootstrapDiagnostics& storageDiagnostics);
  const RuntimeSnapshot& snapshot() const;

 private:
  RuntimeSnapshot snapshot_ = {};
};

}  // namespace v3::runtime
