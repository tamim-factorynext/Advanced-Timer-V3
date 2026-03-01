#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct V3DiRuntimeConfig {
  uint32_t debounceTimeMs;
  cardMode edgeMode;
};

struct V3DiRuntimeState {
  bool logicalState;
  bool physicalState;
  bool triggerFlag;
  uint32_t currentValue;
  uint32_t startOnMs;
  uint32_t startOffMs;
  uint32_t repeatCounter;
  cardState state;
};

struct V3DiStepInput {
  uint32_t nowMs;
  bool sample;
  bool setCondition;
  bool resetCondition;
  bool prevSample;
  bool prevSampleValid;
};

struct V3DiStepOutput {
  bool nextPrevSample;
  bool nextPrevSampleValid;
  bool setResult;
  bool resetResult;
  bool resetOverride;
};

void runV3DiStep(const V3DiRuntimeConfig& cfg, V3DiRuntimeState& runtime,
                 const V3DiStepInput& in, V3DiStepOutput& out);
