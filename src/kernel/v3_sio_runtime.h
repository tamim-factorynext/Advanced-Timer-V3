#pragma once

#include <stdint.h>

#include "kernel/v3_do_runtime.h"

struct V3SioRuntimeConfig {
  cardMode mode;
  bool invert;
  uint32_t delayBeforeOnMs;
  uint32_t activeDurationMs;
  uint32_t repeatCount;
};

struct V3SioRuntimeState {
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
  uint32_t startOnMs;
  uint32_t startOffMs;
  uint32_t repeatCounter;
  cardState state;
};

struct V3SioStepInput {
  uint32_t nowMs;
  bool setCondition;
  bool resetCondition;
};

struct V3SioStepOutput {
  bool setConditionMet;
  bool resetConditionMet;
  bool resetOverride;
  bool effectiveOutput;
};

void runV3SioStep(const V3SioRuntimeConfig& cfg, V3SioRuntimeState& runtime,
                  const V3SioStepInput& in, V3SioStepOutput& out);

