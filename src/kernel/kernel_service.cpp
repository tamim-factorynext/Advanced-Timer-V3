/*
File: src/kernel/kernel_service.cpp
Purpose: Implements the kernel service module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/runtime/runtime_service.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/kernel_service.h"

#include <string.h>

#include "kernel/v3_status_runtime.h"

namespace v3::kernel {

namespace {

/** @brief Clears per-family count fields before recomputing typed summary metrics. */
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

/**
 * @brief Derives family-count summary metrics from validated typed config.
 * @details Computes enabled/family totals and consistency flags used by runtime snapshot projection.
 */
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

/** @brief Maps storage family enum to legacy logic card type token. */
logicCardType logicTypeFromFamily(v3::storage::CardFamily family) {
  switch (family) {
    case v3::storage::CardFamily::DI:
      return DigitalInput;
    case v3::storage::CardFamily::DO:
      return DigitalOutput;
    case v3::storage::CardFamily::AI:
      return AnalogInput;
    case v3::storage::CardFamily::SIO:
      return SoftIO;
    case v3::storage::CardFamily::MATH:
      return MathCard;
    case v3::storage::CardFamily::RTC:
      return RtcCard;
    default:
      return DigitalInput;
  }
}

/** @brief Resolves runtime snapshot mode value for one storage card row. */
cardMode modeFromCardConfig(const v3::storage::CardConfig& card) {
  switch (card.family) {
    case v3::storage::CardFamily::DI:
      if (card.di.edgeMode == 1) return Mode_DI_Falling;
      if (card.di.edgeMode == 2) return Mode_DI_Change;
      return Mode_DI_Rising;
    case v3::storage::CardFamily::DO:
      return static_cast<cardMode>(card.dout.mode);
    case v3::storage::CardFamily::AI:
      return Mode_AI_Continuous;
    case v3::storage::CardFamily::SIO:
      return static_cast<cardMode>(card.sio.mode);
    case v3::storage::CardFamily::MATH:
    case v3::storage::CardFamily::RTC:
      return Mode_None;
    default:
      return Mode_None;
  }
}

/** @brief Returns family-local index of card among enabled cards of same family. */
uint8_t familyLocalIndex(const v3::storage::SystemConfig& system,
                         uint8_t cardPos, v3::storage::CardFamily family) {
  uint8_t idx = 0;
  for (uint8_t i = 0; i < cardPos; ++i) {
    if (!system.cards[i].enabled) continue;
    if (system.cards[i].family == family) {
      idx += 1;
    }
  }
  return idx;
}

}  // namespace

/**
 * @brief Initializes kernel runtime using validated storage config.
 * @details Binds all family slots, resets scan counters, and captures platform dependency.
 */
void KernelService::begin(const v3::storage::ValidatedConfig& config,
                          v3::platform::PlatformService& platform) {
  config_ = config;
  platform_ = &platform;
  metrics_.scanPeriodMs = config_.system.scanPeriodMs;
  metrics_.configuredCardCount = config_.system.cardCount;
  bindTypedConfigSummary(config_, metrics_);
  metrics_.mode = RUN_NORMAL;
  metrics_.stepAppliedCount = 0;
  metrics_.completedScans = 0;
  metrics_.lastScanMs = 0;
  metrics_.diTotalQualifiedEdges = 0;
  metrics_.diInhibitedCount = 0;
  metrics_.diForcedCount = 0;
  metrics_.aiForcedCount = 0;
  metrics_.doActiveCount = 0;
  metrics_.sioActiveCount = 0;
  bindAiSlotsFromConfig();
  bindDiSlotsFromConfig();
  bindDoSlotsFromConfig();
  bindSioSlotsFromConfig();
  bindMathSlotsFromConfig();
  bindRtcSlotsFromConfig();
  nextScanDueMs_ = 0;
  stepPending_ = false;
}

/**
 * @brief Executes one deterministic scan opportunity.
 * @details Applies run-mode/period gating, runs family scan passes, and updates scan metrics.
 */
void KernelService::tick(uint32_t nowMs) {
  if (metrics_.scanPeriodMs == 0) return;

  if (metrics_.mode == RUN_STEP && !stepPending_) return;
  if (nowMs < nextScanDueMs_) return;

  metrics_.completedScans += 1;
  if (metrics_.mode == RUN_STEP) {
    metrics_.stepAppliedCount += 1;
    stepPending_ = false;
  }
  runAiScan();
  runDiScan(nowMs);
  runDoScan(nowMs);
  runSioScan(nowMs);
  runMathScan();
  runRtcScan(nowMs);
  metrics_.lastScanMs = nowMs;
  nextScanDueMs_ = nowMs + metrics_.scanPeriodMs;
}

/** @brief Updates active run mode and clears step token when leaving step mode. */
void KernelService::setRunMode(engineMode mode) {
  metrics_.mode = mode;
  if (mode != RUN_STEP) stepPending_ = false;
}

/** @brief Requests one step token for next eligible step-mode scan. */
void KernelService::requestStepOnce() { stepPending_ = true; }

bool KernelService::setDiForce(uint8_t cardId, bool forceActive, bool forcedSample) {
  for (uint8_t i = 0; i < diSlotCount_; ++i) {
    DiSlot& slot = diSlots_[i];
    if (!slot.active || slot.cardId != cardId) continue;
    slot.forceActive = forceActive;
    slot.forcedSample = forcedSample;
    return true;
  }
  return false;
}

bool KernelService::setAiForce(uint8_t cardId, bool forceActive,
                               uint32_t forcedValue) {
  for (uint8_t i = 0; i < aiSlotCount_; ++i) {
    AiSlot& slot = aiSlots_[i];
    if (!slot.active || slot.cardId != cardId) continue;
    slot.forceActive = forceActive;
    slot.forcedValue = forcedValue;
    return true;
  }
  return false;
}

/** @brief Returns latest kernel metrics snapshot. */
const KernelMetrics& KernelService::metrics() const { return metrics_; }

/**
 * @brief Exports runtime snapshot rows for enabled configured cards.
 * @details Projects per-family slot state into unified `RuntimeSnapshotCard` transport rows.
 */
uint8_t KernelService::exportRuntimeSnapshotCards(RuntimeSnapshotCard* outCards,
                                                  uint8_t capacity) const {
  if (outCards == nullptr || capacity == 0) return 0;

  uint8_t written = 0;
  for (uint8_t i = 0; i < config_.system.cardCount && written < capacity; ++i) {
    const v3::storage::CardConfig& card = config_.system.cards[i];
    if (!card.enabled) continue;

    RuntimeSnapshotCard out = {};
    out.id = card.id;
    out.type = logicTypeFromFamily(card.family);
    out.mode = modeFromCardConfig(card);

    switch (card.family) {
      case v3::storage::CardFamily::DI: {
        out.index = card.di.channel;
        for (uint8_t s = 0; s < diSlotCount_; ++s) {
          const DiSlot& slot = diSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.commandState = slot.state.commandState;
          out.actualState = slot.state.actualState;
          out.edgePulse = slot.state.edgePulse;
          out.state = slot.state.state;
          out.liveValue = slot.state.liveValue;
          out.startOnMs = slot.state.startOnMs;
          out.startOffMs = slot.state.startOffMs;
          out.repeatCounter = slot.state.repeatCounter;
          out.turnOnConditionMet = slot.lastSetResult;
          out.turnOffConditionMet = slot.lastTurnOffConditionMet;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::DO: {
        out.index = card.dout.channel;
        for (uint8_t s = 0; s < doSlotCount_; ++s) {
          const DoSlot& slot = doSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.commandState = slot.state.commandState;
          out.actualState = slot.state.actualState;
          out.edgePulse = slot.state.edgePulse;
          out.state = slot.state.state;
          out.liveValue = slot.state.liveValue;
          out.startOnMs = slot.state.startOnMs;
          out.startOffMs = slot.state.startOffMs;
          out.repeatCounter = slot.state.repeatCounter;
          out.turnOnConditionMet = slot.lastSetResult;
          out.turnOffConditionMet = slot.lastTurnOffConditionMet;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::AI: {
        out.index = card.ai.channel;
        for (uint8_t s = 0; s < aiSlotCount_; ++s) {
          const AiSlot& slot = aiSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.state = slot.state.state;
          out.mode = slot.state.mode;
          out.liveValue = slot.state.liveValue;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::SIO: {
        out.index = familyLocalIndex(config_.system, i, card.family);
        for (uint8_t s = 0; s < sioSlotCount_; ++s) {
          const SioSlot& slot = sioSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.commandState = slot.state.commandState;
          out.actualState = slot.state.actualState;
          out.edgePulse = slot.state.edgePulse;
          out.state = slot.state.state;
          out.liveValue = slot.state.liveValue;
          out.startOnMs = slot.state.startOnMs;
          out.startOffMs = slot.state.startOffMs;
          out.repeatCounter = slot.state.repeatCounter;
          out.turnOnConditionMet = slot.lastSetResult;
          out.turnOffConditionMet = slot.lastTurnOffConditionMet;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::MATH: {
        out.index = familyLocalIndex(config_.system, i, card.family);
        for (uint8_t s = 0; s < mathSlotCount_; ++s) {
          const MathSlot& slot = mathSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.commandState = slot.state.commandState;
          out.actualState = slot.state.actualState;
          out.edgePulse = slot.state.edgePulse;
          out.state = slot.state.state;
          out.liveValue = slot.state.liveValue;
          out.turnOnConditionMet = slot.lastSetResult;
          out.turnOffConditionMet = slot.lastTurnOffConditionMet;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::RTC: {
        out.index = familyLocalIndex(config_.system, i, card.family);
        for (uint8_t s = 0; s < rtcSlotCount_; ++s) {
          const RtcSlot& slot = rtcSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.commandState = slot.state.commandState;
          out.edgePulse = slot.state.edgePulse;
          out.state = slot.state.state;
          out.mode = slot.state.mode;
          out.startOnMs = slot.state.triggerStartMs;
          break;
        }
        break;
      }
      default:
        break;
    }

    outCards[written++] = out;
  }

  return written;
}

/** @brief Binds AI runtime slots from validated config. */
void KernelService::bindAiSlotsFromConfig() {
  aiSlotCount_ = 0;
  for (uint8_t i = 0; i < v3::storage::kMaxCards; ++i) {
    aiSlots_[i] = {};
  }

  for (uint8_t i = 0; i < config_.system.cardCount; ++i) {
    const v3::storage::CardConfig& card = config_.system.cards[i];
    if (card.family != v3::storage::CardFamily::AI || !card.enabled) continue;
    if (aiSlotCount_ >= v3::storage::kMaxCards) break;

    AiSlot& slot = aiSlots_[aiSlotCount_++];
    slot.active = true;
    slot.cardId = card.id;
    slot.channel = card.ai.channel;
    slot.forceActive = false;
    slot.forcedValue = 0;

    slot.cfg.inputMin = card.ai.inputMin;
    slot.cfg.inputMax = card.ai.inputMax;
    slot.cfg.outputMin = card.ai.outputMin;
    slot.cfg.outputMax = card.ai.outputMax;
    slot.cfg.smoothingFactorPct = card.ai.smoothingFactorPct;

    slot.state = {};
    slot.state.mode = Mode_AI_Continuous;
    slot.state.state = State_AI_Streaming;
    if (platform_ != nullptr) {
      platform_->configureInputPin(slot.channel);
    }
  }
}

/** @brief Executes AI family scan pass and updates force counters. */
void KernelService::runAiScan() {
  uint8_t forced = 0;
  for (uint8_t i = 0; i < aiSlotCount_; ++i) {
    AiSlot& slot = aiSlots_[i];
    if (!slot.active) continue;

    const uint32_t sampled =
        (platform_ != nullptr) ? platform_->readAnalogInput(slot.channel) : 0U;
    V3AiStepInput in = {};
    in.rawSample = slot.forceActive ? slot.forcedValue : sampled;
    runV3AiStep(slot.cfg, slot.state, in);
    if (slot.forceActive) forced += 1;
  }
  metrics_.aiForcedCount = forced;
}

/** @brief Binds DI runtime slots from validated config. */
void KernelService::bindDiSlotsFromConfig() {
  diSlotCount_ = 0;
  for (uint8_t i = 0; i < v3::storage::kMaxCards; ++i) {
    diSlots_[i] = {};
  }

  for (uint8_t i = 0; i < config_.system.cardCount; ++i) {
    const v3::storage::CardConfig& card = config_.system.cards[i];
    if (card.family != v3::storage::CardFamily::DI || !card.enabled) continue;
    if (diSlotCount_ >= v3::storage::kMaxCards) break;

    DiSlot& slot = diSlots_[diSlotCount_++];
    slot.active = true;
    slot.cardId = card.id;
    slot.channel = card.di.channel;
    slot.turnOnCondition = card.di.turnOnCondition;
    slot.turnOffCondition = card.di.turnOffCondition;
    slot.forceActive = false;
    slot.forcedSample = false;
    slot.prevSample = false;
    slot.prevSampleValid = false;
    slot.lastSetResult = false;
    slot.lastTurnOffConditionMet = false;

    slot.cfg.debounceTimeMs = card.di.debounceMs;
    slot.cfg.invert = card.di.invert;
    switch (card.di.edgeMode) {
      case 1:
        slot.cfg.edgeMode = Mode_DI_Falling;
        break;
      case 2:
        slot.cfg.edgeMode = Mode_DI_Change;
        break;
      default:
        slot.cfg.edgeMode = Mode_DI_Rising;
        break;
    }

    slot.state = {};
    slot.state.state = State_DI_Idle;
    if (platform_ != nullptr) {
      platform_->configureInputPin(slot.channel);
    }
  }
}

/** @brief Binds DO runtime slots from validated config. */
void KernelService::bindDoSlotsFromConfig() {
  doSlotCount_ = 0;
  for (uint8_t i = 0; i < v3::storage::kMaxCards; ++i) {
    doSlots_[i] = {};
  }

  for (uint8_t i = 0; i < config_.system.cardCount; ++i) {
    const v3::storage::CardConfig& card = config_.system.cards[i];
    if (card.family != v3::storage::CardFamily::DO || !card.enabled) continue;
    if (doSlotCount_ >= v3::storage::kMaxCards) break;

    DoSlot& slot = doSlots_[doSlotCount_++];
    slot.active = true;
    slot.cardId = card.id;
    slot.channel = card.dout.channel;
    slot.turnOnCondition = card.dout.turnOnCondition;
    slot.turnOffCondition = card.dout.turnOffCondition;

    slot.cfg.mode = static_cast<cardMode>(card.dout.mode);
    slot.cfg.invert = card.dout.invert;
    slot.cfg.delayBeforeOnMs = card.dout.delayBeforeOnMs;
    slot.cfg.activeDurationMs = card.dout.activeDurationMs;
    slot.cfg.repeatCount = card.dout.repeatCount;
    slot.lastSetResult = false;
    slot.lastTurnOffConditionMet = false;

    slot.state = {};
    slot.state.state = State_DO_Idle;
    slot.state.actualState = slot.cfg.invert;
    if (platform_ != nullptr) {
      platform_->configureOutputPin(slot.channel);
      platform_->writeDigitalOutput(slot.channel, slot.state.actualState);
    }
  }
}

/** @brief Binds SIO runtime slots from validated config. */
void KernelService::bindSioSlotsFromConfig() {
  sioSlotCount_ = 0;
  for (uint8_t i = 0; i < v3::storage::kMaxCards; ++i) {
    sioSlots_[i] = {};
  }

  for (uint8_t i = 0; i < config_.system.cardCount; ++i) {
    const v3::storage::CardConfig& card = config_.system.cards[i];
    if (card.family != v3::storage::CardFamily::SIO || !card.enabled) continue;
    if (sioSlotCount_ >= v3::storage::kMaxCards) break;

    SioSlot& slot = sioSlots_[sioSlotCount_++];
    slot.active = true;
    slot.cardId = card.id;
    slot.turnOnCondition = card.sio.turnOnCondition;
    slot.turnOffCondition = card.sio.turnOffCondition;
    slot.cfg.mode = static_cast<cardMode>(card.sio.mode);
    slot.cfg.invert = card.sio.invert;
    slot.cfg.delayBeforeOnMs = card.sio.delayBeforeOnMs;
    slot.cfg.activeDurationMs = card.sio.activeDurationMs;
    slot.cfg.repeatCount = card.sio.repeatCount;
    slot.lastSetResult = false;
    slot.lastTurnOffConditionMet = false;
    slot.state = {};
    slot.state.state = State_DO_Idle;
    slot.state.actualState = slot.cfg.invert;
  }
}

/** @brief Binds MATH runtime slots from validated config. */
void KernelService::bindMathSlotsFromConfig() {
  mathSlotCount_ = 0;
  for (uint8_t i = 0; i < v3::storage::kMaxCards; ++i) {
    mathSlots_[i] = {};
  }

  for (uint8_t i = 0; i < config_.system.cardCount; ++i) {
    const v3::storage::CardConfig& card = config_.system.cards[i];
    if (card.family != v3::storage::CardFamily::MATH || !card.enabled) continue;
    if (mathSlotCount_ >= v3::storage::kMaxCards) break;

    MathSlot& slot = mathSlots_[mathSlotCount_++];
    slot.active = true;
    slot.cardId = card.id;
    slot.turnOnCondition = card.math.turnOnCondition;
    slot.turnOffCondition = card.math.turnOffCondition;

    slot.cfg.operation = card.math.operation;
    slot.cfg.inputA = card.math.inputA;
    slot.cfg.inputB = card.math.inputB;
    slot.cfg.inputMin = card.math.inputMin;
    slot.cfg.inputMax = card.math.inputMax;
    slot.cfg.outputMin = card.math.outputMin;
    slot.cfg.outputMax = card.math.outputMax;
    slot.cfg.smoothingFactorPct = card.math.smoothingFactorPct;
    slot.cfg.fallbackValue = card.math.fallbackValue;
    slot.lastSetResult = false;
    slot.lastTurnOffConditionMet = false;

    slot.state = {};
    slot.state.liveValue = slot.cfg.fallbackValue;
    slot.state.state = State_None;
    slot.state.commandState = false;
    slot.state.actualState = false;
    slot.state.edgePulse = false;
  }
}

/** @brief Binds RTC runtime slots from validated config. */
void KernelService::bindRtcSlotsFromConfig() {
  rtcSlotCount_ = 0;
  for (uint8_t i = 0; i < v3::storage::kMaxCards; ++i) {
    rtcSlots_[i] = {};
  }

  for (uint8_t i = 0; i < config_.system.cardCount; ++i) {
    const v3::storage::CardConfig& card = config_.system.cards[i];
    if (card.family != v3::storage::CardFamily::RTC || !card.enabled) continue;
    if (rtcSlotCount_ >= v3::storage::kMaxCards) break;

    RtcSlot& slot = rtcSlots_[rtcSlotCount_++];
    slot.active = true;
    slot.cardId = card.id;
    slot.schedule.enabled = true;
    slot.schedule.hasYear = card.rtc.hasYear;
    slot.schedule.year = card.rtc.year;
    slot.schedule.hasMonth = card.rtc.hasMonth;
    slot.schedule.month = card.rtc.month;
    slot.schedule.hasDay = card.rtc.hasDay;
    slot.schedule.day = card.rtc.day;
    slot.schedule.hasWeekday = card.rtc.hasWeekday;
    slot.schedule.weekday = card.rtc.weekday;
    slot.schedule.hasHour = card.rtc.hasHour;
    slot.schedule.hour = card.rtc.hour;
    slot.schedule.minute = card.rtc.minute;
    slot.schedule.rtcCardId = card.id;
    slot.cfg.triggerDurationMs = card.rtc.triggerDurationMs;
    slot.state = {};
    slot.state.mode = Mode_None;
    slot.state.state = State_None;
    slot.lastMinuteKeyValid = false;
    slot.lastMinuteKey = 0;
  }
}

/**
 * @brief Builds transient runtime signal array used by condition evaluators.
 * @details Merges all active family slot states into card-id indexed signal rows.
 */
void KernelService::buildSignalSnapshot(V3RuntimeSignal* signals,
                                        uint8_t signalCount) const {
  if (signals == nullptr) return;
  for (uint8_t i = 0; i < diSlotCount_; ++i) {
    const DiSlot& slot = diSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = DigitalInput;
    signal.state = slot.state.state;
    signal.commandState = slot.state.commandState;
    signal.actualState = slot.state.actualState;
    signal.edgePulse = slot.state.edgePulse;
    signal.liveValue = slot.state.liveValue;
  }
  for (uint8_t i = 0; i < aiSlotCount_; ++i) {
    const AiSlot& slot = aiSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = AnalogInput;
    signal.state = slot.state.state;
    signal.commandState = false;
    signal.actualState = false;
    signal.edgePulse = false;
    signal.liveValue = slot.state.liveValue;
  }
  for (uint8_t i = 0; i < doSlotCount_; ++i) {
    const DoSlot& slot = doSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = DigitalOutput;
    signal.state = slot.state.state;
    signal.commandState = slot.state.commandState;
    signal.actualState = slot.state.actualState;
    signal.edgePulse = slot.state.edgePulse;
    signal.liveValue = slot.state.liveValue;
  }
  for (uint8_t i = 0; i < sioSlotCount_; ++i) {
    const SioSlot& slot = sioSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = SoftIO;
    signal.state = slot.state.state;
    signal.commandState = slot.state.commandState;
    signal.actualState = slot.state.actualState;
    signal.edgePulse = slot.state.edgePulse;
    signal.liveValue = slot.state.liveValue;
  }
  for (uint8_t i = 0; i < mathSlotCount_; ++i) {
    const MathSlot& slot = mathSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = MathCard;
    signal.state = slot.state.state;
    signal.commandState = false;
    signal.actualState = false;
    signal.edgePulse = slot.state.edgePulse;
    signal.liveValue = slot.state.liveValue;
  }
  for (uint8_t i = 0; i < rtcSlotCount_; ++i) {
    const RtcSlot& slot = rtcSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = RtcCard;
    signal.state = slot.state.state;
    signal.commandState = slot.state.commandState;
    signal.actualState = false;
    signal.edgePulse = slot.state.edgePulse;
    signal.liveValue = 0;
  }
}

/**
 * @brief Executes DI family pass including condition evaluation and edge qualification.
 * @details Updates DI counters/inhibition/force telemetry and slot state.
 */
void KernelService::runDiScan(uint32_t nowMs) {
  uint32_t totalEdges = 0;
  uint8_t inhibited = 0;
  uint8_t forced = 0;
  V3RuntimeSignal signals[v3::storage::kMaxCards] = {};

  buildSignalSnapshot(signals, v3::storage::kMaxCards);

  for (uint8_t i = 0; i < diSlotCount_; ++i) {
    DiSlot& slot = diSlots_[i];
    if (!slot.active) continue;

    const bool sampled =
        (platform_ != nullptr) ? platform_->readDigitalInput(slot.channel) : false;

    V3DiStepInput in = {};
    in.nowMs = nowMs;
    in.sample = sampled;
    in.forceActive = slot.forceActive;
    in.forcedSample = slot.forcedSample;
    in.turnOnCondition =
        evalConditionBlock(slot.turnOnCondition, signals, v3::storage::kMaxCards);
    in.turnOffCondition =
        evalConditionBlock(slot.turnOffCondition, signals, v3::storage::kMaxCards);
    in.prevSample = slot.prevSample;
    in.prevSampleValid = slot.prevSampleValid;

    V3DiStepOutput out = {};
    runV3DiStep(slot.cfg, slot.state, in, out);
    slot.lastSetResult = out.turnOnConditionMet;
    slot.lastTurnOffConditionMet = out.turnOffConditionMet;

    slot.prevSample = out.nextPrevSample;
    slot.prevSampleValid = out.nextPrevSampleValid;

    totalEdges += slot.state.liveValue;
    if (slot.state.state == State_DI_Inhibited) {
      inhibited += 1;
    }
    if (slot.forceActive) {
      forced += 1;
    }

    if (slot.cardId < v3::storage::kMaxCards) {
      V3RuntimeSignal& signal = signals[slot.cardId];
      signal.type = DigitalInput;
      signal.state = slot.state.state;
      signal.commandState = slot.state.commandState;
      signal.actualState = slot.state.actualState;
      signal.edgePulse = slot.state.edgePulse;
      signal.liveValue = slot.state.liveValue;
    }
  }

  metrics_.diTotalQualifiedEdges = totalEdges;
  metrics_.diInhibitedCount = inhibited;
  metrics_.diForcedCount = forced;
}

/** @brief Executes DO family pass including condition evaluation and output writes. */
void KernelService::runDoScan(uint32_t nowMs) {
  uint8_t activeCount = 0;
  V3RuntimeSignal signals[v3::storage::kMaxCards] = {};

  for (uint8_t i = 0; i < doSlotCount_; ++i) {
    DoSlot& slot = doSlots_[i];
    if (!slot.active) continue;

    memset(signals, 0, sizeof(signals));
    buildSignalSnapshot(signals, v3::storage::kMaxCards);

    V3DoStepInput in = {};
    in.nowMs = nowMs;
    in.turnOnCondition =
        evalConditionBlock(slot.turnOnCondition, signals, v3::storage::kMaxCards);
    in.turnOffCondition =
        evalConditionBlock(slot.turnOffCondition, signals, v3::storage::kMaxCards);

    V3DoStepOutput out = {};
    runV3DoStep(slot.cfg, slot.state, in, out);
    slot.lastSetResult = out.turnOnConditionMet;
    slot.lastTurnOffConditionMet = out.turnOffConditionMet;
    if (platform_ != nullptr) {
      platform_->writeDigitalOutput(slot.channel, slot.state.actualState);
    }
    if (slot.state.state == State_DO_OnDelay || slot.state.state == State_DO_Active) {
      activeCount += 1;
    }
  }

  metrics_.doActiveCount = activeCount;
}

/** @brief Executes SIO family pass including condition evaluation. */
void KernelService::runSioScan(uint32_t nowMs) {
  uint8_t activeCount = 0;
  V3RuntimeSignal signals[v3::storage::kMaxCards] = {};

  for (uint8_t i = 0; i < sioSlotCount_; ++i) {
    SioSlot& slot = sioSlots_[i];
    if (!slot.active) continue;

    memset(signals, 0, sizeof(signals));
    buildSignalSnapshot(signals, v3::storage::kMaxCards);

    V3SioStepInput in = {};
    in.nowMs = nowMs;
    in.turnOnCondition =
        evalConditionBlock(slot.turnOnCondition, signals, v3::storage::kMaxCards);
    in.turnOffCondition =
        evalConditionBlock(slot.turnOffCondition, signals, v3::storage::kMaxCards);

    V3SioStepOutput out = {};
    runV3SioStep(slot.cfg, slot.state, in, out);
    slot.lastSetResult = out.turnOnConditionMet;
    slot.lastTurnOffConditionMet = out.turnOffConditionMet;
    if (slot.state.state == State_DO_OnDelay || slot.state.state == State_DO_Active) {
      activeCount += 1;
    }
  }

  metrics_.sioActiveCount = activeCount;
}

/** @brief Executes MATH family pass including condition evaluation. */
void KernelService::runMathScan() {
  V3RuntimeSignal signals[v3::storage::kMaxCards] = {};

  for (uint8_t i = 0; i < mathSlotCount_; ++i) {
    MathSlot& slot = mathSlots_[i];
    if (!slot.active) continue;

    memset(signals, 0, sizeof(signals));
    buildSignalSnapshot(signals, v3::storage::kMaxCards);

    V3MathStepInput in = {};
    in.turnOnCondition =
        evalConditionBlock(slot.turnOnCondition, signals, v3::storage::kMaxCards);
    in.turnOffCondition =
        evalConditionBlock(slot.turnOffCondition, signals, v3::storage::kMaxCards);

    V3MathStepOutput out = {};
    runV3MathStep(slot.cfg, slot.state, in, out);
    slot.lastSetResult = out.turnOnConditionMet;
    slot.lastTurnOffConditionMet = out.turnOffConditionMet;
  }
}

/**
 * @brief Executes RTC family pass using minute-stamp schedule matching.
 * @details Applies minute-key deduplication to prevent repeated trigger within same minute.
 */
void KernelService::runRtcScan(uint32_t nowMs) {
  v3::platform::LocalMinuteStamp stamp = {};
  const bool hasValidTime =
      (platform_ != nullptr) && platform_->readLocalMinuteStamp(stamp);

  for (uint8_t i = 0; i < rtcSlotCount_; ++i) {
    RtcSlot& slot = rtcSlots_[i];
    if (!slot.active) continue;

    V3RtcStepInput in = {};
    in.nowMs = nowMs;
    runV3RtcStep(slot.cfg, slot.state, in);

    if (!hasValidTime) continue;

    V3RtcMinuteStamp rtcStamp = {};
    rtcStamp.year = stamp.year;
    rtcStamp.month = stamp.month;
    rtcStamp.day = stamp.day;
    rtcStamp.weekday = stamp.weekday;
    rtcStamp.hour = stamp.hour;
    rtcStamp.minute = stamp.minute;

    if (!v3RtcChannelMatchesMinute(slot.schedule, rtcStamp)) continue;

    const uint32_t minuteKey = v3RtcMinuteKey(rtcStamp);
    if (slot.lastMinuteKeyValid && minuteKey == slot.lastMinuteKey) continue;

    slot.state.commandState = true;
    slot.state.edgePulse = true;
    slot.state.triggerStartMs = nowMs;
    slot.lastMinuteKey = minuteKey;
    slot.lastMinuteKeyValid = true;
  }
}

/** @brief Evaluates one condition block against runtime signal snapshot. */
bool KernelService::evalConditionBlock(const v3::storage::ConditionBlock& block,
                                       const V3RuntimeSignal* signals,
                                       uint8_t signalCount) const {
  const bool a = evalConditionClause(block.clauseA, signals, signalCount);
  if (block.combiner == v3::storage::ConditionCombiner::None) return a;
  const bool b = evalConditionClause(block.clauseB, signals, signalCount);
  if (block.combiner == v3::storage::ConditionCombiner::And) return a && b;
  if (block.combiner == v3::storage::ConditionCombiner::Or) return a || b;
  return false;
}

/**
 * @brief Evaluates one condition clause against runtime signal snapshot.
 * @details Supports state, trigger, numeric threshold, and mission-lifecycle operators.
 */
bool KernelService::evalConditionClause(const v3::storage::ConditionClause& clause,
                                        const V3RuntimeSignal* signals,
                                        uint8_t signalCount) const {
  if (signals == nullptr || clause.sourceCardId >= signalCount) return false;
  const V3RuntimeSignal& s = signals[clause.sourceCardId];
  switch (clause.op) {
    case v3::storage::ConditionOperator::AlwaysTrue:
      return true;
    case v3::storage::ConditionOperator::AlwaysFalse:
      return false;
    case v3::storage::ConditionOperator::LogicalTrue:
      return s.commandState;
    case v3::storage::ConditionOperator::LogicalFalse:
      return !s.commandState;
    case v3::storage::ConditionOperator::PhysicalOn:
      return s.actualState;
    case v3::storage::ConditionOperator::PhysicalOff:
      return !s.actualState;
    case v3::storage::ConditionOperator::Triggered:
      return s.edgePulse;
    case v3::storage::ConditionOperator::TriggerCleared:
      return !s.edgePulse;
    case v3::storage::ConditionOperator::GT:
      return s.liveValue > clause.threshold;
    case v3::storage::ConditionOperator::LT:
      return s.liveValue < clause.threshold;
    case v3::storage::ConditionOperator::EQ:
      return s.liveValue == clause.threshold;
    case v3::storage::ConditionOperator::NEQ:
      return s.liveValue != clause.threshold;
    case v3::storage::ConditionOperator::GTE:
      return s.liveValue >= clause.threshold;
    case v3::storage::ConditionOperator::LTE:
      return s.liveValue <= clause.threshold;
    case v3::storage::ConditionOperator::Running:
      return isMissionRunning(s.type, s.state);
    case v3::storage::ConditionOperator::Finished:
      return isMissionFinished(s.type, s.state);
    case v3::storage::ConditionOperator::Stopped:
      return isMissionStopped(s.type, s.state);
    default:
      return false;
  }
}

}  // namespace v3::kernel



