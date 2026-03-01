#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct V3MathRuntimeConfig {
  uint32_t inputA;
  uint32_t inputB;
  uint32_t fallbackValue;
  uint32_t clampMin;
  uint32_t clampMax;
  bool clampEnabled;
};

struct V3MathRuntimeState {
  bool logicalState;
  bool physicalState;
  bool triggerFlag;
  uint32_t currentValue;
  cardState state;
};

struct V3MathStepInput {
  bool setCondition;
  bool resetCondition;
};

struct V3MathStepOutput {
  bool setResult;
  bool resetResult;
  bool resetOverride;
};

void runV3MathStep(const V3MathRuntimeConfig& cfg, V3MathRuntimeState& runtime,
                   const V3MathStepInput& in, V3MathStepOutput& out);

