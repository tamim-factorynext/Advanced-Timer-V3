#pragma once

#include <stdint.h>

#include "control/command_dto.h"
#include "kernel/v3_di_runtime.h"
#include "kernel/v3_runtime_signals.h"
#include "storage/v3_config_contract.h"
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
  uint32_t diTotalQualifiedEdges;
  uint8_t diInhibitedCount;
  uint8_t diForcedCount;
};

class KernelService {
 public:
  void begin(const v3::storage::ValidatedConfig& config);
  void tick(uint32_t nowMs);
  void setRunMode(runMode mode);
  void requestStepOnce();
  bool setDiForce(uint8_t cardId, bool forceActive, bool forcedSample);
  const KernelMetrics& metrics() const;

 private:
  struct DiSlot {
    bool active;
    uint8_t cardId;
    uint8_t channel;
    v3::storage::ConditionBlock setCondition;
    v3::storage::ConditionBlock resetCondition;
    bool forceActive;
    bool forcedSample;
    bool prevSample;
    bool prevSampleValid;
    V3DiRuntimeConfig cfg;
    V3DiRuntimeState state;
  };

  void bindDiSlotsFromConfig();
  void runDiScan(uint32_t nowMs);
  bool evalConditionBlock(const v3::storage::ConditionBlock& block,
                          const V3RuntimeSignal* signals,
                          uint8_t signalCount) const;
  bool evalConditionClause(const v3::storage::ConditionClause& clause,
                           const V3RuntimeSignal* signals,
                           uint8_t signalCount) const;

  v3::storage::ValidatedConfig config_ = {};
  KernelMetrics metrics_ = {};
  DiSlot diSlots_[v3::storage::kMaxCards] = {};
  uint8_t diSlotCount_ = 0;
  uint32_t nextScanDueMs_ = 0;
  bool stepPending_ = false;
};

}  // namespace v3::kernel
