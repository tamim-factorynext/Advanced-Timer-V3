/*
File: src/kernel/kernel_service.h
Purpose: Declares the kernel service module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/main.cpp
- src/runtime/runtime_service.h
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "control/command_dto.h"
#include "kernel/v3_ai_runtime.h"
#include "kernel/v3_di_runtime.h"
#include "kernel/v3_do_runtime.h"
#include "kernel/v3_math_runtime.h"
#include "kernel/v3_rtc_runtime.h"
#include "kernel/v3_runtime_signals.h"
#include "kernel/v3_sio_runtime.h"
#include "platform/platform_service.h"
#include "runtime/runtime_snapshot_card.h"
#include "storage/v3_config_contract.h"
#include "storage/v3_config_validator.h"

namespace v3::kernel {

/**
 * @brief Deterministic scan-loop metrics exported by kernel runtime.
 * @details Aggregates family counts, run-mode state, scan counters, and card-family activity telemetry.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 * - src/runtime/runtime_service.cpp
 * - src/main.cpp
 */
struct KernelMetrics {
  uint32_t scanPeriodMs;
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
  engineMode mode;
  uint32_t stepAppliedCount;
  uint32_t completedScans;
  uint32_t lastScanMs;
  uint32_t scanLastUs;
  uint32_t scanMaxUs;
  uint32_t scanOverrunCount;
  bool scanOverrunLast;
  uint32_t diTotalQualifiedEdges;
  uint8_t diInhibitedCount;
  uint8_t diForcedCount;
  uint8_t aiForcedCount;
  uint8_t doActiveCount;
  uint8_t sioActiveCount;
};

/**
 * @brief Owns deterministic card runtime execution on kernel core.
 * @details Applies queued commands, executes family scan passes, and exports per-card runtime snapshots.
 * @par Used By
 * - src/main.cpp
 * - src/runtime/runtime_service.h
 */
class KernelService {
 public:
  /**
   * @brief Initializes kernel runtime using validated config and platform boundary.
   * @details Binds card-family slots and resets scan/telemetry state.
   * @param config Validated storage config contract.
   * @param platform Platform service used for deterministic hardware interactions.
   * @par Used By
   * - src/main.cpp
   */
  void begin(const v3::storage::ValidatedConfig& config,
             v3::platform::PlatformService& platform);
  /**
   * @brief Executes one deterministic scan opportunity.
   * @details Honors run mode and scan-period gating before running family passes.
   * @param nowMs Current deterministic loop timestamp in milliseconds.
   * @par Used By
   * - src/main.cpp
   */
  void tick(uint32_t nowMs);
  /**
   * @brief Updates kernel run mode.
   * @details Affects scan gating semantics (`RUN_NORMAL`, `RUN_STEP`, `RUN_BREAKPOINT`).
   * @param mode Desired engine mode.
   * @par Used By
   * - src/main.cpp
   */
  void setRunMode(engineMode mode);
  /**
   * @brief Requests one step execution when in step mode.
   * @details Marks one pending step token consumed by next eligible tick.
   * @par Used By
   * - src/main.cpp
   */
  void requestStepOnce();
  /**
   * @brief Sets DI forcing policy for target card.
   * @details Enables/disables forced sample path for one DI card slot.
   * @return `true` when target DI card exists and force state is applied.
   * @par Used By
   * - src/main.cpp
   */
  bool setDiForce(uint8_t cardId, bool forceActive, bool forcedSample);
  /**
   * @brief Sets AI forcing policy for target card.
   * @details Enables/disables forced numeric value path for one AI card slot.
   * @return `true` when target AI card exists and force state is applied.
   * @par Used By
   * - src/main.cpp
   */
  bool setAiForce(uint8_t cardId, bool forceActive, uint32_t forcedValue);
  /**
   * @brief Returns current kernel metrics snapshot.
   * @details Read-only telemetry view used by runtime snapshot projection.
   * @par Used By
   * - src/main.cpp
   * - src/runtime/runtime_service.cpp
   */
  const KernelMetrics& metrics() const;
  /**
   * @brief Exports per-card runtime snapshot rows.
   * @details Projects active card-family states into transport-ready snapshot DTOs.
   * @param outCards Output array receiving snapshot entries.
   * @param capacity Max writable entries in `outCards`.
   * @return Number of cards written to output.
   * @par Used By
   * - src/main.cpp
   * - src/runtime/snapshot_card_builder.cpp
   */
  uint8_t exportRuntimeSnapshotCards(RuntimeSnapshotCard* outCards,
                                     uint8_t capacity) const;

 private:
  struct DiSlot {
    bool active;
    uint8_t cardId;
    uint8_t channel;
    v3::storage::ConditionBlock turnOnCondition;
    v3::storage::ConditionBlock turnOffCondition;
    bool forceActive;
    bool forcedSample;
    bool prevSample;
    bool prevSampleValid;
    V3DiRuntimeConfig cfg;
    V3DiRuntimeState state;
    bool lastSetResult;
    bool lastTurnOffConditionMet;
  };

  struct AiSlot {
    bool active;
    uint8_t cardId;
    uint8_t channel;
    bool forceActive;
    uint32_t forcedValue;
    V3AiRuntimeConfig cfg;
    V3AiRuntimeState state;
  };

  struct DoSlot {
    bool active;
    uint8_t cardId;
    uint8_t channel;
    v3::storage::ConditionBlock turnOnCondition;
    v3::storage::ConditionBlock turnOffCondition;
    V3DoRuntimeConfig cfg;
    V3DoRuntimeState state;
    bool lastSetResult;
    bool lastTurnOffConditionMet;
  };

  struct SioSlot {
    bool active;
    uint8_t cardId;
    v3::storage::ConditionBlock turnOnCondition;
    v3::storage::ConditionBlock turnOffCondition;
    V3SioRuntimeConfig cfg;
    V3SioRuntimeState state;
    bool lastSetResult;
    bool lastTurnOffConditionMet;
  };

  struct MathSlot {
    bool active;
    uint8_t cardId;
    v3::storage::ConditionBlock turnOnCondition;
    v3::storage::ConditionBlock turnOffCondition;
    V3MathRuntimeConfig cfg;
    V3MathRuntimeState state;
    bool lastSetResult;
    bool lastTurnOffConditionMet;
  };

  struct RtcSlot {
    bool active;
    uint8_t cardId;
    V3RtcScheduleView schedule;
    V3RtcRuntimeConfig cfg;
    V3RtcRuntimeState state;
    bool lastMinuteKeyValid;
    uint32_t lastMinuteKey;
  };

  void bindDiSlotsFromConfig();
  void bindAiSlotsFromConfig();
  void bindDoSlotsFromConfig();
  void bindSioSlotsFromConfig();
  void bindMathSlotsFromConfig();
  void bindRtcSlotsFromConfig();
  void runAiScan();
  void runDiScan(uint32_t nowMs);
  void runDoScan(uint32_t nowMs);
  void runSioScan(uint32_t nowMs);
  void runMathScan();
  void runRtcScan(uint32_t nowMs);
  void buildSignalSnapshot(V3RuntimeSignal* signals, uint8_t signalCount) const;
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
  AiSlot aiSlots_[v3::storage::kMaxCards] = {};
  uint8_t aiSlotCount_ = 0;
  DoSlot doSlots_[v3::storage::kMaxCards] = {};
  uint8_t doSlotCount_ = 0;
  SioSlot sioSlots_[v3::storage::kMaxCards] = {};
  uint8_t sioSlotCount_ = 0;
  MathSlot mathSlots_[v3::storage::kMaxCards] = {};
  uint8_t mathSlotCount_ = 0;
  RtcSlot rtcSlots_[v3::storage::kMaxCards] = {};
  uint8_t rtcSlotCount_ = 0;
  V3RuntimeSignal signalScratch_[v3::storage::kMaxCards] = {};
  uint32_t nextScanDueMs_ = 0;
  bool stepPending_ = false;
  v3::platform::PlatformService* platform_ = nullptr;
};

}  // namespace v3::kernel



