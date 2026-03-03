/*
File: src/kernel/v3_sio_runtime.h
Purpose: Declares the v3 sio runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src\kernel\kernel_service.h
- src\kernel\v3_runtime_adapters.h
- src\kernel\v3_runtime_store.h
- src\kernel\v3_sio_runtime.cpp
- (+ more call sites)

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
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
  bool turnOnCondition;
  bool turnOffCondition;
};

struct V3SioStepOutput {
  bool turnOnConditionMet;
  bool turnOffConditionMet;
  bool resetOverride;
  bool effectiveOutput;
};

void runV3SioStep(const V3SioRuntimeConfig& cfg, V3SioRuntimeState& runtime,
                  const V3SioStepInput& in, V3SioStepOutput& out);



