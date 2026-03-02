#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct V3DoRuntimeConfig {
  cardMode mode;
  bool invert;
  uint32_t delayBeforeOnMs;
  uint32_t activeDurationMs;
  uint32_t repeatCount;
};

struct V3DoRuntimeState {
  bool logicalState;
  bool physicalState;
  bool triggerFlag;
  uint32_t currentValue;
  uint32_t startOnMs;
  uint32_t startOffMs;
  uint32_t repeatCounter;
  cardState state;
};

struct V3DoStepInput {
  uint32_t nowMs;
  bool setCondition;
  bool resetCondition;
};

struct V3DoStepOutput {
  bool setResult;
  bool resetResult;
  bool resetOverride;
  bool effectiveOutput;
};

void runV3DoStep(const V3DoRuntimeConfig& cfg, V3DoRuntimeState& runtime,
                 const V3DoStepInput& in, V3DoStepOutput& out);
