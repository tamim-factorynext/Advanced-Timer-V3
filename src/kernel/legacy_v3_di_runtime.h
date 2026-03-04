/*
File: src/kernel/legacy_v3_di_runtime.h
Purpose: Declares the v3 di runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/kernel/legacy_v3_di_runtime.cpp
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

/** @brief DI runtime configuration contract for one card slot. */
struct V3DiRuntimeConfig {
  uint32_t debounceTimeMs;
  cardMode edgeMode;
  bool invert;
};

/** @brief Mutable DI runtime state projected into snapshots/signals. */
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

/** @brief DI step input bundle for one scan pass. */
struct V3DiStepInput {
  uint32_t nowMs;
  bool sample;
  bool forcedSample;
  bool forceActive;
  bool turnOnCondition;
  bool turnOffCondition;
  bool prevSample;
  bool prevSampleValid;
};

/** @brief DI step output bundle including edge/condition outcomes. */
struct V3DiStepOutput {
  bool nextPrevSample;
  bool nextPrevSampleValid;
  bool turnOnConditionMet;
  bool turnOffConditionMet;
  bool resetOverride;
};

/**
 * @brief Executes one DI runtime step.
 * @details Applies debounce, edge mode, and turn-on/turn-off condition gating.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
void runV3DiStep(const V3DiRuntimeConfig& cfg, V3DiRuntimeState& runtime,
                 const V3DiStepInput& in, V3DiStepOutput& out);





