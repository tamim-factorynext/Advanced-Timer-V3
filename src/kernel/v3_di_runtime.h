#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct V3DiRuntimeConfig {
  uint32_t debounceTimeMs;
  cardMode edgeMode;
  bool invert;
};

struct V3DiRuntimeState {
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
  uint32_t startOnMs;
  uint32_t startOffMs;
  uint32_t repeatCounter;
  cardState state;
};

struct V3DiStepInput {
  uint32_t nowMs;
  bool sample;
  bool forcedSample;
  bool forceActive;
  bool setCondition;
  bool resetCondition;
  bool prevSample;
  bool prevSampleValid;
};

struct V3DiStepOutput {
  bool nextPrevSample;
  bool nextPrevSampleValid;
  bool setConditionMet;
  bool resetConditionMet;
  bool resetOverride;
};

void runV3DiStep(const V3DiRuntimeConfig& cfg, V3DiRuntimeState& runtime,
                 const V3DiStepInput& in, V3DiStepOutput& out);

