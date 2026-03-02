#pragma once

#include <stdint.h>

#include "control/command_dto.h"
#include "storage/v3_config_validator.h"

namespace v3::kernel {

struct KernelMetrics {
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
  uint32_t completedScans;
  uint32_t lastScanMs;
};

class KernelService {
 public:
  void begin(const v3::storage::ValidatedConfig& config);
  void tick(uint32_t nowMs);
  void setRunMode(runMode mode);
  void requestStepOnce();
  const KernelMetrics& metrics() const;

 private:
  v3::storage::ValidatedConfig config_ = {};
  KernelMetrics metrics_ = {};
  uint32_t nextScanDueMs_ = 0;
  bool stepPending_ = false;
};

}  // namespace v3::kernel
