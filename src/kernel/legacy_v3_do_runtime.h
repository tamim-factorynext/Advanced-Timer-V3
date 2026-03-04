/*
File: src/kernel/legacy_v3_do_runtime.h
Purpose: Declares the v3 do runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/kernel/legacy_v3_runtime_adapters.h
- src/kernel/legacy_v3_runtime_store.h
- src/kernel/legacy_v3_sio_runtime.h
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

/** @brief DO runtime configuration contract for one card slot. */
struct V3DoRuntimeConfig {
  cardMode mode;
  bool invert;
  uint32_t delayBeforeOnMs;
  uint32_t activeDurationMs;
  uint32_t repeatCount;
};

/** @brief Mutable DO runtime state projected into snapshots/signals. */
struct V3DoRuntimeState {
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
  uint32_t startOnMs;
  uint32_t startOffMs;
  uint32_t repeatCounter;
  cardState state;
};

/** @brief DO step input bundle for one scan pass. */
struct V3DoStepInput {
  uint32_t nowMs;
  bool turnOnCondition;
  bool turnOffCondition;
};

/** @brief DO step output bundle including condition outcomes and effective output. */
struct V3DoStepOutput {
  bool turnOnConditionMet;
  bool turnOffConditionMet;
  bool resetOverride;
  bool effectiveOutput;
};

/**
 * @brief Executes one DO runtime step.
 * @details Applies mode/timing state machine with turn-on/turn-off condition gating.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 * - src/kernel/legacy_v3_sio_runtime.cpp
 */
void runV3DoStep(const V3DoRuntimeConfig& cfg, V3DoRuntimeState& runtime,
                 const V3DoStepInput& in, V3DoStepOutput& out);





