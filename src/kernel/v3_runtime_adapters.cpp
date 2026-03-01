#include "kernel/v3_runtime_adapters.h"

V3DiRuntimeConfig makeDiRuntimeConfig(const LogicCard& card) {
  V3DiRuntimeConfig cfg = {};
  cfg.debounceTimeMs = card.setting1;
  cfg.edgeMode = card.mode;
  return cfg;
}

V3DiRuntimeState makeDiRuntimeState(const LogicCard& card) {
  V3DiRuntimeState runtime = {};
  runtime.logicalState = card.logicalState;
  runtime.physicalState = card.physicalState;
  runtime.triggerFlag = card.triggerFlag;
  runtime.currentValue = card.currentValue;
  runtime.startOnMs = card.startOnMs;
  runtime.startOffMs = card.startOffMs;
  runtime.repeatCounter = card.repeatCounter;
  runtime.state = card.state;
  return runtime;
}

void applyDiRuntimeState(LogicCard& card, const V3DiRuntimeState& runtime) {
  card.logicalState = runtime.logicalState;
  card.physicalState = runtime.physicalState;
  card.triggerFlag = runtime.triggerFlag;
  card.currentValue = runtime.currentValue;
  card.startOnMs = runtime.startOnMs;
  card.startOffMs = runtime.startOffMs;
  card.repeatCounter = runtime.repeatCounter;
  card.state = runtime.state;
}

V3AiRuntimeConfig makeAiRuntimeConfig(const LogicCard& card) {
  V3AiRuntimeConfig cfg = {};
  cfg.inputMin = card.setting1;
  cfg.inputMax = card.setting2;
  cfg.outputMin = card.startOnMs;
  cfg.outputMax = card.startOffMs;
  cfg.emaAlphaX1000 = card.setting3;
  return cfg;
}

V3AiRuntimeState makeAiRuntimeState(const LogicCard& card) {
  V3AiRuntimeState runtime = {};
  runtime.currentValue = card.currentValue;
  runtime.mode = card.mode;
  runtime.state = card.state;
  return runtime;
}

void applyAiRuntimeState(LogicCard& card, const V3AiRuntimeState& runtime) {
  card.currentValue = runtime.currentValue;
  card.mode = runtime.mode;
  card.state = runtime.state;
}

V3DoRuntimeConfig makeDoRuntimeConfig(const LogicCard& card) {
  V3DoRuntimeConfig cfg = {};
  cfg.mode = card.mode;
  cfg.delayBeforeOnMs = card.setting1;
  cfg.onDurationMs = card.setting2;
  cfg.repeatCount = card.setting3;
  return cfg;
}

V3DoRuntimeState makeDoRuntimeState(const LogicCard& card) {
  V3DoRuntimeState runtime = {};
  runtime.logicalState = card.logicalState;
  runtime.physicalState = card.physicalState;
  runtime.triggerFlag = card.triggerFlag;
  runtime.currentValue = card.currentValue;
  runtime.startOnMs = card.startOnMs;
  runtime.startOffMs = card.startOffMs;
  runtime.repeatCounter = card.repeatCounter;
  runtime.state = card.state;
  return runtime;
}

void applyDoRuntimeState(LogicCard& card, const V3DoRuntimeState& runtime) {
  card.logicalState = runtime.logicalState;
  card.physicalState = runtime.physicalState;
  card.triggerFlag = runtime.triggerFlag;
  card.currentValue = runtime.currentValue;
  card.startOnMs = runtime.startOnMs;
  card.startOffMs = runtime.startOffMs;
  card.repeatCounter = runtime.repeatCounter;
  card.state = runtime.state;
}

V3SioRuntimeConfig makeSioRuntimeConfig(const LogicCard& card) {
  V3SioRuntimeConfig cfg = {};
  cfg.mode = card.mode;
  cfg.delayBeforeOnMs = card.setting1;
  cfg.onDurationMs = card.setting2;
  cfg.repeatCount = card.setting3;
  return cfg;
}

V3SioRuntimeState makeSioRuntimeState(const LogicCard& card) {
  V3SioRuntimeState runtime = {};
  runtime.logicalState = card.logicalState;
  runtime.physicalState = card.physicalState;
  runtime.triggerFlag = card.triggerFlag;
  runtime.currentValue = card.currentValue;
  runtime.startOnMs = card.startOnMs;
  runtime.startOffMs = card.startOffMs;
  runtime.repeatCounter = card.repeatCounter;
  runtime.state = card.state;
  return runtime;
}

void applySioRuntimeState(LogicCard& card, const V3SioRuntimeState& runtime) {
  card.logicalState = runtime.logicalState;
  card.physicalState = runtime.physicalState;
  card.triggerFlag = runtime.triggerFlag;
  card.currentValue = runtime.currentValue;
  card.startOnMs = runtime.startOnMs;
  card.startOffMs = runtime.startOffMs;
  card.repeatCounter = runtime.repeatCounter;
  card.state = runtime.state;
}

V3MathRuntimeConfig makeMathRuntimeConfig(const LogicCard& card) {
  V3MathRuntimeConfig cfg = {};
  cfg.inputA = card.setting1;
  cfg.inputB = card.setting2;
  cfg.fallbackValue = card.setting3;
  cfg.clampMin = card.startOnMs;
  cfg.clampMax = card.startOffMs;
  cfg.clampEnabled = (card.startOffMs >= card.startOnMs);
  return cfg;
}

V3MathRuntimeState makeMathRuntimeState(const LogicCard& card) {
  V3MathRuntimeState runtime = {};
  runtime.logicalState = card.logicalState;
  runtime.physicalState = card.physicalState;
  runtime.triggerFlag = card.triggerFlag;
  runtime.currentValue = card.currentValue;
  runtime.state = card.state;
  return runtime;
}

void applyMathRuntimeState(LogicCard& card, const V3MathRuntimeState& runtime) {
  card.logicalState = runtime.logicalState;
  card.physicalState = runtime.physicalState;
  card.triggerFlag = runtime.triggerFlag;
  card.currentValue = runtime.currentValue;
  card.state = runtime.state;
}

V3RtcRuntimeConfig makeRtcRuntimeConfig(const LogicCard& card) {
  V3RtcRuntimeConfig cfg = {};
  cfg.triggerDurationMs = card.setting1;
  return cfg;
}

V3RtcRuntimeState makeRtcRuntimeState(const LogicCard& card) {
  V3RtcRuntimeState runtime = {};
  runtime.logicalState = card.logicalState;
  runtime.physicalState = card.physicalState;
  runtime.triggerFlag = card.triggerFlag;
  runtime.currentValue = card.currentValue;
  runtime.triggerStartMs = card.startOnMs;
  runtime.mode = card.mode;
  runtime.state = card.state;
  return runtime;
}

void applyRtcRuntimeState(LogicCard& card, const V3RtcRuntimeState& runtime) {
  card.logicalState = runtime.logicalState;
  card.physicalState = runtime.physicalState;
  card.triggerFlag = runtime.triggerFlag;
  card.currentValue = runtime.currentValue;
  card.startOnMs = runtime.triggerStartMs;
  card.mode = runtime.mode;
  card.state = runtime.state;
}

