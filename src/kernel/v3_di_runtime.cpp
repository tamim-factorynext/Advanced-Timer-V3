#include "kernel/v3_di_runtime.h"

namespace {
void resetDiCounter(V3DiRuntimeState& runtime) {
  runtime.triggerFlag = false;
  runtime.currentValue = 0;
  runtime.startOnMs = 0;
  runtime.startOffMs = 0;
}
}  // namespace

void runV3DiStep(const V3DiRuntimeConfig& cfg, V3DiRuntimeState& runtime,
                 const V3DiStepInput& in, V3DiStepOutput& out) {
  const bool rawSample = in.forceActive ? in.forcedSample : in.sample;
  const bool effectiveSample = cfg.invert ? !rawSample : rawSample;

  out.setResult = in.setCondition;
  out.resetResult = in.resetCondition;
  out.resetOverride = in.setCondition && in.resetCondition;
  out.nextPrevSample = effectiveSample;
  out.nextPrevSampleValid = true;

  // Physical state is polarity-adjusted signal value and is independent from
  // set/reset gating.
  runtime.physicalState = effectiveSample;

  if (in.resetCondition) {
    // Reset path only clears counter/filter state and inhibits processing.
    resetDiCounter(runtime);
    runtime.state = State_DI_Inhibited;
    return;
  }

  if (!in.setCondition) {
    runtime.triggerFlag = false;
    runtime.state = State_DI_Idle;
    return;
  }

  bool previousSample = effectiveSample;
  if (in.prevSampleValid) {
    previousSample = in.prevSample;
  }

  if (runtime.state == State_DI_Filtering) {
    const bool pendingSample = (runtime.startOffMs != 0U);
    if (effectiveSample != pendingSample) {
      runtime.triggerFlag = false;
      runtime.state = State_DI_Idle;
      return;
    }

    const uint32_t elapsed = in.nowMs - runtime.startOnMs;
    if (cfg.debounceTimeMs > 0 && elapsed < cfg.debounceTimeMs) {
      runtime.triggerFlag = false;
      runtime.state = State_DI_Filtering;
      return;
    }

    runtime.triggerFlag = true;
    runtime.currentValue += 1;
    runtime.logicalState = effectiveSample;
    runtime.state = State_DI_Qualified;
    runtime.startOnMs = in.nowMs;
    return;
  }

  const bool risingEdge = (!previousSample && effectiveSample);
  const bool fallingEdge = (previousSample && !effectiveSample);
  bool edgeMatchesMode = false;
  switch (cfg.edgeMode) {
    case Mode_DI_Rising:
      edgeMatchesMode = risingEdge;
      break;
    case Mode_DI_Falling:
      edgeMatchesMode = fallingEdge;
      break;
    case Mode_DI_Change:
      edgeMatchesMode = risingEdge || fallingEdge;
      break;
    default:
      edgeMatchesMode = false;
      break;
  }

  if (!edgeMatchesMode) {
    runtime.triggerFlag = false;
    runtime.state = State_DI_Idle;
    return;
  }

  runtime.startOnMs = in.nowMs;
  runtime.startOffMs = effectiveSample ? 1U : 0U;
  if (cfg.debounceTimeMs == 0) {
    runtime.triggerFlag = true;
    runtime.currentValue += 1;
    runtime.logicalState = effectiveSample;
    runtime.state = State_DI_Qualified;
    return;
  }

  runtime.triggerFlag = false;
  runtime.state = State_DI_Filtering;
}
