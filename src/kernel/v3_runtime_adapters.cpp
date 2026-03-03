/*
File: src/kernel/v3_runtime_adapters.cpp
Purpose: Implements the v3 runtime adapters module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src\kernel\v3_runtime_adapters.cpp
- src\kernel\v3_runtime_adapters.h
- src\kernel\v3_runtime_store.cpp

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/v3_runtime_adapters.h"
#include "kernel/legacy_card_fields.h"

V3DiRuntimeConfig makeDiRuntimeConfig(const LogicCard& card) {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = legacyDiDebounceMs(card);
  cfg.edgeMode = card.mode;
  cfg.invert = card.invert;
  return cfg;
}

V3DiRuntimeState makeDiRuntimeState(const LogicCard& card) {
  V3DiRuntimeState runtime = {};
  runtime.commandState = card.commandState;
  runtime.actualState = card.actualState;
  runtime.edgePulse = card.edgePulse;
  runtime.liveValue = card.liveValue;
  runtime.startOnMs = card.startOnMs;
  runtime.startOffMs = card.startOffMs;
  runtime.repeatCounter = card.repeatCounter;
  runtime.state = card.state;
  return runtime;
}

void applyDiRuntimeState(LogicCard& card, const V3DiRuntimeState& runtime) {
  card.commandState = runtime.commandState;
  card.actualState = runtime.actualState;
  card.edgePulse = runtime.edgePulse;
  card.liveValue = runtime.liveValue;
  card.startOnMs = runtime.startOnMs;
  card.startOffMs = runtime.startOffMs;
  card.repeatCounter = runtime.repeatCounter;
  card.state = runtime.state;
}

V3AiRuntimeConfig makeAiRuntimeConfig(const LogicCard& card) {
  V3AiRuntimeConfig cfg = {};
  cfg.inputMin = legacyAiInputMin(card);
  cfg.inputMax = legacyAiInputMax(card);
  cfg.outputMin = legacyAiOutputMin(card);
  cfg.outputMax = legacyAiOutputMax(card);
  cfg.smoothingFactorPct = legacyAiAlphaX1000(card) / 10U;
  return cfg;
}

V3AiRuntimeState makeAiRuntimeState(const LogicCard& card) {
  V3AiRuntimeState runtime = {};
  runtime.liveValue = card.liveValue;
  runtime.mode = card.mode;
  runtime.state = card.state;
  return runtime;
}

void applyAiRuntimeState(LogicCard& card, const V3AiRuntimeState& runtime) {
  card.liveValue = runtime.liveValue;
  card.mode = runtime.mode;
  card.state = runtime.state;
}

V3DoRuntimeConfig makeDoRuntimeConfig(const LogicCard& card) {
  V3DoRuntimeConfig cfg = {};
  cfg.mode = card.mode;
  cfg.invert = card.invert;
  cfg.delayBeforeOnMs = legacyDoDelayBeforeOnMs(card);
  cfg.activeDurationMs = legacyDoOnDurationMs(card);
  cfg.repeatCount = legacyDoRepeatCount(card);
  return cfg;
}

V3DoRuntimeState makeDoRuntimeState(const LogicCard& card) {
  V3DoRuntimeState runtime = {};
  runtime.commandState = card.commandState;
  runtime.actualState = card.actualState;
  runtime.edgePulse = card.edgePulse;
  runtime.liveValue = card.liveValue;
  runtime.startOnMs = card.startOnMs;
  runtime.startOffMs = card.startOffMs;
  runtime.repeatCounter = card.repeatCounter;
  runtime.state = card.state;
  return runtime;
}

void applyDoRuntimeState(LogicCard& card, const V3DoRuntimeState& runtime) {
  card.commandState = runtime.commandState;
  card.actualState = runtime.actualState;
  card.edgePulse = runtime.edgePulse;
  card.liveValue = runtime.liveValue;
  card.startOnMs = runtime.startOnMs;
  card.startOffMs = runtime.startOffMs;
  card.repeatCounter = runtime.repeatCounter;
  card.state = runtime.state;
}

V3SioRuntimeConfig makeSioRuntimeConfig(const LogicCard& card) {
  V3SioRuntimeConfig cfg = {};
  cfg.mode = card.mode;
  cfg.invert = card.invert;
  cfg.delayBeforeOnMs = legacyDoDelayBeforeOnMs(card);
  cfg.activeDurationMs = legacyDoOnDurationMs(card);
  cfg.repeatCount = legacyDoRepeatCount(card);
  return cfg;
}

V3SioRuntimeState makeSioRuntimeState(const LogicCard& card) {
  V3SioRuntimeState runtime = {};
  runtime.commandState = card.commandState;
  runtime.actualState = card.actualState;
  runtime.edgePulse = card.edgePulse;
  runtime.liveValue = card.liveValue;
  runtime.startOnMs = card.startOnMs;
  runtime.startOffMs = card.startOffMs;
  runtime.repeatCounter = card.repeatCounter;
  runtime.state = card.state;
  return runtime;
}

void applySioRuntimeState(LogicCard& card, const V3SioRuntimeState& runtime) {
  card.commandState = runtime.commandState;
  card.actualState = runtime.actualState;
  card.edgePulse = runtime.edgePulse;
  card.liveValue = runtime.liveValue;
  card.startOnMs = runtime.startOnMs;
  card.startOffMs = runtime.startOffMs;
  card.repeatCounter = runtime.repeatCounter;
  card.state = runtime.state;
}

V3MathRuntimeConfig makeMathRuntimeConfig(const LogicCard& card) {
  V3MathRuntimeConfig cfg = {};
  cfg.operation = 0U;
  cfg.inputA = legacyMathInputA(card);
  cfg.inputB = legacyMathInputB(card);
  cfg.inputMin = legacyMathClampMin(card);
  cfg.inputMax = legacyMathClampMax(card);
  if (cfg.inputMin >= cfg.inputMax) {
    cfg.inputMin = 0U;
    cfg.inputMax = 10000U;
  }
  cfg.outputMin = cfg.inputMin;
  cfg.outputMax = cfg.inputMax;
  cfg.smoothingFactorPct = 100U;
  cfg.fallbackValue = legacyMathFallbackValue(card);
  return cfg;
}

V3MathRuntimeState makeMathRuntimeState(const LogicCard& card) {
  V3MathRuntimeState runtime = {};
  runtime.commandState = card.commandState;
  runtime.actualState = card.actualState;
  runtime.edgePulse = card.edgePulse;
  runtime.liveValue = card.liveValue;
  runtime.state = card.state;
  return runtime;
}

void applyMathRuntimeState(LogicCard& card, const V3MathRuntimeState& runtime) {
  card.commandState = runtime.commandState;
  card.actualState = runtime.actualState;
  card.edgePulse = runtime.edgePulse;
  card.liveValue = runtime.liveValue;
  card.state = runtime.state;
}

V3RtcRuntimeConfig makeRtcRuntimeConfig(const LogicCard& card) {
  V3RtcRuntimeConfig cfg = {};
  cfg.triggerDurationMs = legacyRtcTriggerDurationMs(card);
  return cfg;
}

V3RtcRuntimeState makeRtcRuntimeState(const LogicCard& card) {
  V3RtcRuntimeState runtime = {};
  runtime.commandState = card.commandState;
  runtime.actualState = card.actualState;
  runtime.edgePulse = card.edgePulse;
  runtime.liveValue = card.liveValue;
  runtime.triggerStartMs = card.startOnMs;
  runtime.mode = card.mode;
  runtime.state = card.state;
  return runtime;
}

void applyRtcRuntimeState(LogicCard& card, const V3RtcRuntimeState& runtime) {
  card.commandState = runtime.commandState;
  card.actualState = runtime.actualState;
  card.edgePulse = runtime.edgePulse;
  card.liveValue = runtime.liveValue;
  card.startOnMs = runtime.triggerStartMs;
  card.mode = runtime.mode;
  card.state = runtime.state;
}

