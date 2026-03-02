#include "kernel/v3_ai_runtime.h"

namespace {
uint32_t clampUInt32(uint32_t value, uint32_t lo, uint32_t hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}
}  // namespace

void runV3AiStep(const V3AiRuntimeConfig& cfg, V3AiRuntimeState& runtime,
                 const V3AiStepInput& in) {
  const uint32_t inMin = (cfg.inputMin < cfg.inputMax) ? cfg.inputMin : cfg.inputMax;
  const uint32_t inMax = (cfg.inputMin < cfg.inputMax) ? cfg.inputMax : cfg.inputMin;
  const uint32_t clamped = clampUInt32(in.rawSample, inMin, inMax);

  uint32_t scaled = cfg.outputMin;
  if (inMax != inMin) {
    const int64_t outMin = static_cast<int64_t>(cfg.outputMin);
    const int64_t outMax = static_cast<int64_t>(cfg.outputMax);
    const int64_t outDelta = outMax - outMin;
    const int64_t inDelta = static_cast<int64_t>(inMax - inMin);
    const int64_t inOffset = static_cast<int64_t>(clamped - inMin);
    int64_t mapped = outMin + ((inOffset * outDelta) / inDelta);
    if (mapped < 0) mapped = 0;
    scaled = static_cast<uint32_t>(mapped);
  }

  const uint32_t alpha = (cfg.emaAlphaX100 > 100U) ? 100U : cfg.emaAlphaX100;
  const uint64_t filtered =
      ((static_cast<uint64_t>(alpha) * scaled) +
       (static_cast<uint64_t>(100U - alpha) * runtime.currentValue)) /
      100ULL;
  runtime.currentValue = static_cast<uint32_t>(filtered);
  runtime.mode = Mode_AI_Continuous;
  runtime.state = State_AI_Streaming;
}
