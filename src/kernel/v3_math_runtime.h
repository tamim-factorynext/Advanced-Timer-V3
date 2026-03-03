#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct V3MathRuntimeConfig {
  uint8_t operation;
  uint32_t inputA;
  uint32_t inputB;
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t smoothingFactorPct;
  uint32_t fallbackValue;
};

struct V3MathRuntimeState {
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
  cardState state;
};

struct V3MathStepInput {
  bool setCondition;
  bool resetCondition;
};

struct V3MathStepOutput {
  bool setConditionMet;
  bool resetConditionMet;
  bool resetOverride;
};

void runV3MathStep(const V3MathRuntimeConfig& cfg, V3MathRuntimeState& runtime,
                   const V3MathStepInput& in, V3MathStepOutput& out);

