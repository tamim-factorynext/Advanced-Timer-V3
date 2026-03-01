#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct V3AiRuntimeConfig {
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t emaAlphaX1000;
};

struct V3AiRuntimeState {
  uint32_t currentValue;
  cardMode mode;
  cardState state;
};

struct V3AiStepInput {
  uint32_t rawSample;
};

void runV3AiStep(const V3AiRuntimeConfig& cfg, V3AiRuntimeState& runtime,
                 const V3AiStepInput& in);

