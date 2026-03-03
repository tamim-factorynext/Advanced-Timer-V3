/*
File: src/kernel/v3_math_runtime.h
Purpose: Declares the v3 math runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src\kernel\kernel_service.h
- src\kernel\v3_math_runtime.cpp
- src\kernel\v3_math_runtime.h
- src\kernel\v3_runtime_adapters.h
- (+ more call sites)

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
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
  bool turnOnCondition;
  bool turnOffCondition;
};

struct V3MathStepOutput {
  bool turnOnConditionMet;
  bool turnOffConditionMet;
  bool resetOverride;
};

void runV3MathStep(const V3MathRuntimeConfig& cfg, V3MathRuntimeState& runtime,
                   const V3MathStepInput& in, V3MathStepOutput& out);



