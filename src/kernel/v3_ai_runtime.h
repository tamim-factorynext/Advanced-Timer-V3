/*
File: src/kernel/v3_ai_runtime.h
Purpose: Declares the v3 ai runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/kernel/v3_runtime_adapters.h
- src/kernel/v3_runtime_store.h
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

/** @brief AI runtime configuration contract for one card slot. */
struct V3AiRuntimeConfig {
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t smoothingFactorPct;
};

/** @brief Mutable AI runtime state projected into snapshots/signals. */
struct V3AiRuntimeState {
  uint32_t liveValue;
  cardMode mode;
  cardState state;
};

/** @brief Input sample payload for one AI step evaluation. */
struct V3AiStepInput {
  uint32_t rawSample;
};

/**
 * @brief Executes one AI runtime step.
 * @details Applies input/output scaling and smoothing to produce updated `liveValue`.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
void runV3AiStep(const V3AiRuntimeConfig& cfg, V3AiRuntimeState& runtime,
                 const V3AiStepInput& in);

