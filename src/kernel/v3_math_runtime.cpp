#include "kernel/v3_math_runtime.h"

#include <limits.h>

namespace {
uint32_t clampUInt32(uint32_t value, uint32_t lo, uint32_t hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}
}  // namespace

void runV3MathStep(const V3MathRuntimeConfig& cfg, V3MathRuntimeState& runtime,
                   const V3MathStepInput& in, V3MathStepOutput& out) {
  out.setResult = in.setCondition;
  out.resetResult = in.resetCondition;
  out.resetOverride = in.setCondition && in.resetCondition;

  if (in.resetCondition) {
    runtime.logicalState = false;
    runtime.physicalState = false;
    runtime.triggerFlag = false;
    runtime.currentValue = cfg.fallbackValue;
    runtime.state = State_None;
    return;
  }

  if (!in.setCondition) {
    runtime.logicalState = false;
    runtime.physicalState = false;
    runtime.triggerFlag = false;
    runtime.state = State_None;
    return;
  }

  runtime.logicalState = true;
  runtime.physicalState = true;
  runtime.triggerFlag = true;

  const uint64_t raw =
      static_cast<uint64_t>(cfg.inputA) + static_cast<uint64_t>(cfg.inputB);
  uint32_t value =
      (raw > static_cast<uint64_t>(UINT32_MAX)) ? UINT32_MAX : static_cast<uint32_t>(raw);
  if (cfg.clampEnabled) {
    value = clampUInt32(value, cfg.clampMin, cfg.clampMax);
  }
  runtime.currentValue = value;
  runtime.state = State_None;
}

