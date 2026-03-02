#include "kernel/v3_math_runtime.h"

#include <limits.h>

namespace {
constexpr uint8_t kMathOpAdd = 0;
constexpr uint8_t kMathOpSubSat = 1;
constexpr uint8_t kMathOpMul = 2;
constexpr uint8_t kMathOpDivSafe = 3;

uint32_t clampToRange(uint32_t value, uint32_t lo, uint32_t hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

uint32_t saturatingAdd(uint32_t a, uint32_t b) {
  const uint64_t raw = static_cast<uint64_t>(a) + static_cast<uint64_t>(b);
  return (raw > static_cast<uint64_t>(UINT32_MAX)) ? UINT32_MAX
                                                    : static_cast<uint32_t>(raw);
}

uint32_t saturatingMul(uint32_t a, uint32_t b) {
  const uint64_t raw = static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
  return (raw > static_cast<uint64_t>(UINT32_MAX)) ? UINT32_MAX
                                                    : static_cast<uint32_t>(raw);
}

uint32_t computeRawValue(const V3MathRuntimeConfig& cfg) {
  switch (cfg.operation) {
    case kMathOpAdd:
      return saturatingAdd(cfg.inputA, cfg.inputB);
    case kMathOpSubSat:
      return (cfg.inputA >= cfg.inputB) ? (cfg.inputA - cfg.inputB) : 0U;
    case kMathOpMul:
      return saturatingMul(cfg.inputA, cfg.inputB);
    case kMathOpDivSafe:
      if (cfg.inputB == 0U) return cfg.fallbackValue;
      return cfg.inputA / cfg.inputB;
    default:
      return cfg.fallbackValue;
  }
}

uint32_t mapRange(uint32_t value, uint32_t inMin, uint32_t inMax, uint32_t outMin,
                  uint32_t outMax) {
  const uint32_t spanIn = inMax - inMin;
  if (spanIn == 0U) return outMin;

  const uint32_t offsetIn = value - inMin;
  const bool descending = (outMax < outMin);
  const uint64_t spanOutAbs =
      descending ? static_cast<uint64_t>(outMin - outMax)
                 : static_cast<uint64_t>(outMax - outMin);
  const uint64_t delta = (static_cast<uint64_t>(offsetIn) * spanOutAbs) / spanIn;
  const int64_t mapped = descending
                             ? static_cast<int64_t>(outMin) -
                                   static_cast<int64_t>(delta)
                             : static_cast<int64_t>(outMin) +
                                   static_cast<int64_t>(delta);
  if (mapped <= 0) return 0U;
  if (mapped >= static_cast<int64_t>(UINT32_MAX)) return UINT32_MAX;
  return static_cast<uint32_t>(mapped);
}

uint32_t applyEma(uint32_t previous, uint32_t sample, uint32_t alphaX100) {
  const uint32_t alpha = (alphaX100 > 100U) ? 100U : alphaX100;
  const uint64_t weighted =
      static_cast<uint64_t>(sample) * alpha +
      static_cast<uint64_t>(previous) * (100U - alpha);
  return static_cast<uint32_t>((weighted + 50U) / 100U);
}
}  // namespace

void runV3MathStep(const V3MathRuntimeConfig& cfg, V3MathRuntimeState& runtime,
                   const V3MathStepInput& in, V3MathStepOutput& out) {
  out.setResult = in.setCondition;
  out.resetResult = in.resetCondition;
  out.resetOverride = in.setCondition && in.resetCondition;

  const uint32_t previousValue = runtime.currentValue;
  uint32_t nextValue = previousValue;

  if (in.resetCondition) {
    nextValue = cfg.fallbackValue;
  } else if (in.setCondition) {
    const uint32_t raw = computeRawValue(cfg);
    const uint32_t clampedInput = clampToRange(raw, cfg.inputMin, cfg.inputMax);
    const uint32_t scaled =
        mapRange(clampedInput, cfg.inputMin, cfg.inputMax, cfg.outputMin,
                 cfg.outputMax);
    nextValue = applyEma(previousValue, scaled, cfg.emaAlphaX100);
  }

  runtime.logicalState = false;
  runtime.physicalState = false;
  runtime.triggerFlag = (nextValue != previousValue);
  runtime.currentValue = nextValue;
  runtime.state = State_None;
}
