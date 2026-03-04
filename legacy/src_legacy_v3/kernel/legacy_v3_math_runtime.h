/*
File: src/kernel/legacy_v3_math_runtime.h
Purpose: Declares the v3 math runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/kernel/legacy_v3_math_runtime.cpp
- src/kernel/legacy_v3_runtime_adapters.h
- (+ more call sites)
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

/** @brief MATH runtime configuration contract for one card slot. */
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

/** @brief Mutable MATH runtime state projected into snapshots/signals. */
struct V3MathRuntimeState {
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
  cardState state;
};

/** @brief MATH step input bundle for one scan pass. */
struct V3MathStepInput {
  bool turnOnCondition;
  bool turnOffCondition;
};

/** @brief MATH step output bundle including condition outcomes and reset override. */
struct V3MathStepOutput {
  bool turnOnConditionMet;
  bool turnOffConditionMet;
  bool resetOverride;
};

/**
 * @brief Executes one MATH runtime step.
 * @details Applies selected math operation plus gating/override semantics.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
void runV3MathStep(const V3MathRuntimeConfig& cfg, V3MathRuntimeState& runtime,
                   const V3MathStepInput& in, V3MathStepOutput& out);





