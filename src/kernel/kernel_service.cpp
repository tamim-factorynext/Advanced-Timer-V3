#include "kernel/kernel_service.h"

#include <string.h>

#include "kernel/v3_status_runtime.h"

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

void KernelService::begin(const v3::storage::ValidatedConfig& config,
                          v3::platform::PlatformService& platform) {
  config_ = config;
  platform_ = &platform;
  metrics_.scanIntervalMs = config_.system.scanIntervalMs;
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
  nextScanDueMs_ = 0;
  stepPending_ = false;
}

void KernelService::tick(uint32_t nowMs) {
  if (metrics_.scanIntervalMs == 0) return;

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
  metrics_.lastScanMs = nowMs;
  nextScanDueMs_ = nowMs + metrics_.scanIntervalMs;
}

void KernelService::setRunMode(runMode mode) {
  metrics_.mode = mode;
  if (mode != RUN_STEP) stepPending_ = false;
}

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

const KernelMetrics& KernelService::metrics() const { return metrics_; }

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
          out.logicalState = slot.state.logicalState;
          out.physicalState = slot.state.physicalState;
          out.triggerFlag = slot.state.triggerFlag;
          out.state = slot.state.state;
          out.currentValue = slot.state.currentValue;
          out.startOnMs = slot.state.startOnMs;
          out.startOffMs = slot.state.startOffMs;
          out.repeatCounter = slot.state.repeatCounter;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::DO: {
        out.index = card.dout.channel;
        for (uint8_t s = 0; s < doSlotCount_; ++s) {
          const DoSlot& slot = doSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.logicalState = slot.state.logicalState;
          out.physicalState = slot.state.physicalState;
          out.triggerFlag = slot.state.triggerFlag;
          out.state = slot.state.state;
          out.currentValue = slot.state.currentValue;
          out.startOnMs = slot.state.startOnMs;
          out.startOffMs = slot.state.startOffMs;
          out.repeatCounter = slot.state.repeatCounter;
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
          out.currentValue = slot.state.currentValue;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::SIO: {
        out.index = familyLocalIndex(config_.system, i, card.family);
        for (uint8_t s = 0; s < sioSlotCount_; ++s) {
          const SioSlot& slot = sioSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.logicalState = slot.state.logicalState;
          out.physicalState = slot.state.physicalState;
          out.triggerFlag = slot.state.triggerFlag;
          out.state = slot.state.state;
          out.currentValue = slot.state.currentValue;
          out.startOnMs = slot.state.startOnMs;
          out.startOffMs = slot.state.startOffMs;
          out.repeatCounter = slot.state.repeatCounter;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::MATH: {
        out.index = familyLocalIndex(config_.system, i, card.family);
        for (uint8_t s = 0; s < mathSlotCount_; ++s) {
          const MathSlot& slot = mathSlots_[s];
          if (!slot.active || slot.cardId != card.id) continue;
          out.logicalState = slot.state.logicalState;
          out.physicalState = slot.state.physicalState;
          out.triggerFlag = slot.state.triggerFlag;
          out.state = slot.state.state;
          out.currentValue = slot.state.currentValue;
          break;
        }
        break;
      }
      case v3::storage::CardFamily::RTC: {
        out.index = familyLocalIndex(config_.system, i, card.family);
        out.state = State_None;
        break;
      }
      default:
        break;
    }

    outCards[written++] = out;
  }

  return written;
}

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
    slot.cfg.emaAlphaX100 = card.ai.emaAlphaX100;

    slot.state = {};
    slot.state.mode = Mode_AI_Continuous;
    slot.state.state = State_AI_Streaming;
    if (platform_ != nullptr) {
      platform_->configureInputPin(slot.channel);
    }
  }
}

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
    slot.setCondition = card.di.setCondition;
    slot.resetCondition = card.di.resetCondition;
    slot.forceActive = false;
    slot.forcedSample = false;
    slot.prevSample = false;
    slot.prevSampleValid = false;

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
    slot.setCondition = card.dout.setCondition;
    slot.resetCondition = card.dout.resetCondition;

    slot.cfg.mode = static_cast<cardMode>(card.dout.mode);
    slot.cfg.invert = card.dout.invert;
    slot.cfg.delayBeforeOnMs = card.dout.delayBeforeOnMs;
    slot.cfg.activeDurationMs = card.dout.activeDurationMs;
    slot.cfg.repeatCount = card.dout.repeatCount;

    slot.state = {};
    slot.state.state = State_DO_Idle;
    slot.state.physicalState = slot.cfg.invert;
    if (platform_ != nullptr) {
      platform_->configureOutputPin(slot.channel);
      platform_->writeDigitalOutput(slot.channel, slot.state.physicalState);
    }
  }
}

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
    slot.setCondition = card.sio.setCondition;
    slot.resetCondition = card.sio.resetCondition;
    slot.cfg.mode = static_cast<cardMode>(card.sio.mode);
    slot.cfg.invert = card.sio.invert;
    slot.cfg.delayBeforeOnMs = card.sio.delayBeforeOnMs;
    slot.cfg.activeDurationMs = card.sio.activeDurationMs;
    slot.cfg.repeatCount = card.sio.repeatCount;
    slot.state = {};
    slot.state.state = State_DO_Idle;
    slot.state.physicalState = slot.cfg.invert;
  }
}

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
    slot.setCondition = card.math.setCondition;
    slot.resetCondition = card.math.resetCondition;

    slot.cfg.operation = card.math.operation;
    slot.cfg.inputA = card.math.inputA;
    slot.cfg.inputB = card.math.inputB;
    slot.cfg.inputMin = card.math.inputMin;
    slot.cfg.inputMax = card.math.inputMax;
    slot.cfg.outputMin = card.math.outputMin;
    slot.cfg.outputMax = card.math.outputMax;
    slot.cfg.emaAlphaX100 = card.math.emaAlphaX100;
    slot.cfg.fallbackValue = card.math.fallbackValue;

    slot.state = {};
    slot.state.currentValue = slot.cfg.fallbackValue;
    slot.state.state = State_None;
    slot.state.logicalState = false;
    slot.state.physicalState = false;
    slot.state.triggerFlag = false;
  }
}

void KernelService::buildSignalSnapshot(V3RuntimeSignal* signals,
                                        uint8_t signalCount) const {
  if (signals == nullptr) return;
  for (uint8_t i = 0; i < diSlotCount_; ++i) {
    const DiSlot& slot = diSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = DigitalInput;
    signal.state = slot.state.state;
    signal.logicalState = slot.state.logicalState;
    signal.physicalState = slot.state.physicalState;
    signal.triggerFlag = slot.state.triggerFlag;
    signal.currentValue = slot.state.currentValue;
  }
  for (uint8_t i = 0; i < aiSlotCount_; ++i) {
    const AiSlot& slot = aiSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = AnalogInput;
    signal.state = slot.state.state;
    signal.logicalState = false;
    signal.physicalState = false;
    signal.triggerFlag = false;
    signal.currentValue = slot.state.currentValue;
  }
  for (uint8_t i = 0; i < doSlotCount_; ++i) {
    const DoSlot& slot = doSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = DigitalOutput;
    signal.state = slot.state.state;
    signal.logicalState = slot.state.logicalState;
    signal.physicalState = slot.state.physicalState;
    signal.triggerFlag = slot.state.triggerFlag;
    signal.currentValue = slot.state.currentValue;
  }
  for (uint8_t i = 0; i < sioSlotCount_; ++i) {
    const SioSlot& slot = sioSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = SoftIO;
    signal.state = slot.state.state;
    signal.logicalState = slot.state.logicalState;
    signal.physicalState = slot.state.physicalState;
    signal.triggerFlag = slot.state.triggerFlag;
    signal.currentValue = slot.state.currentValue;
  }
  for (uint8_t i = 0; i < mathSlotCount_; ++i) {
    const MathSlot& slot = mathSlots_[i];
    if (!slot.active || slot.cardId >= signalCount) continue;
    V3RuntimeSignal& signal = signals[slot.cardId];
    signal.type = MathCard;
    signal.state = slot.state.state;
    signal.logicalState = false;
    signal.physicalState = false;
    signal.triggerFlag = slot.state.triggerFlag;
    signal.currentValue = slot.state.currentValue;
  }
}

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
    in.setCondition =
        evalConditionBlock(slot.setCondition, signals, v3::storage::kMaxCards);
    in.resetCondition =
        evalConditionBlock(slot.resetCondition, signals, v3::storage::kMaxCards);
    in.prevSample = slot.prevSample;
    in.prevSampleValid = slot.prevSampleValid;

    V3DiStepOutput out = {};
    runV3DiStep(slot.cfg, slot.state, in, out);

    slot.prevSample = out.nextPrevSample;
    slot.prevSampleValid = out.nextPrevSampleValid;

    totalEdges += slot.state.currentValue;
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
      signal.logicalState = slot.state.logicalState;
      signal.physicalState = slot.state.physicalState;
      signal.triggerFlag = slot.state.triggerFlag;
      signal.currentValue = slot.state.currentValue;
    }
  }

  metrics_.diTotalQualifiedEdges = totalEdges;
  metrics_.diInhibitedCount = inhibited;
  metrics_.diForcedCount = forced;
}

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
    in.setCondition =
        evalConditionBlock(slot.setCondition, signals, v3::storage::kMaxCards);
    in.resetCondition =
        evalConditionBlock(slot.resetCondition, signals, v3::storage::kMaxCards);

    V3DoStepOutput out = {};
    runV3DoStep(slot.cfg, slot.state, in, out);
    if (platform_ != nullptr) {
      platform_->writeDigitalOutput(slot.channel, slot.state.physicalState);
    }
    if (slot.state.state == State_DO_OnDelay || slot.state.state == State_DO_Active) {
      activeCount += 1;
    }
  }

  metrics_.doActiveCount = activeCount;
}

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
    in.setCondition =
        evalConditionBlock(slot.setCondition, signals, v3::storage::kMaxCards);
    in.resetCondition =
        evalConditionBlock(slot.resetCondition, signals, v3::storage::kMaxCards);

    V3SioStepOutput out = {};
    runV3SioStep(slot.cfg, slot.state, in, out);
    if (slot.state.state == State_DO_OnDelay || slot.state.state == State_DO_Active) {
      activeCount += 1;
    }
  }

  metrics_.sioActiveCount = activeCount;
}

void KernelService::runMathScan() {
  V3RuntimeSignal signals[v3::storage::kMaxCards] = {};

  for (uint8_t i = 0; i < mathSlotCount_; ++i) {
    MathSlot& slot = mathSlots_[i];
    if (!slot.active) continue;

    memset(signals, 0, sizeof(signals));
    buildSignalSnapshot(signals, v3::storage::kMaxCards);

    V3MathStepInput in = {};
    in.setCondition =
        evalConditionBlock(slot.setCondition, signals, v3::storage::kMaxCards);
    in.resetCondition =
        evalConditionBlock(slot.resetCondition, signals, v3::storage::kMaxCards);

    V3MathStepOutput out = {};
    runV3MathStep(slot.cfg, slot.state, in, out);
  }
}

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
      return s.logicalState;
    case v3::storage::ConditionOperator::LogicalFalse:
      return !s.logicalState;
    case v3::storage::ConditionOperator::PhysicalOn:
      return s.physicalState;
    case v3::storage::ConditionOperator::PhysicalOff:
      return !s.physicalState;
    case v3::storage::ConditionOperator::Triggered:
      return s.triggerFlag;
    case v3::storage::ConditionOperator::TriggerCleared:
      return !s.triggerFlag;
    case v3::storage::ConditionOperator::GT:
      return s.currentValue > clause.threshold;
    case v3::storage::ConditionOperator::LT:
      return s.currentValue < clause.threshold;
    case v3::storage::ConditionOperator::EQ:
      return s.currentValue == clause.threshold;
    case v3::storage::ConditionOperator::NEQ:
      return s.currentValue != clause.threshold;
    case v3::storage::ConditionOperator::GTE:
      return s.currentValue >= clause.threshold;
    case v3::storage::ConditionOperator::LTE:
      return s.currentValue <= clause.threshold;
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
