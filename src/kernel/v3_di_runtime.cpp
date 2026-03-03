/*
File: src/kernel/v3_di_runtime.cpp
Purpose: Implements the v3 di runtime module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/kernel_service.h
- src/kernel/v3_di_runtime.cpp
- src/kernel/v3_di_runtime.h
- src/kernel/v3_runtime_adapters.h
- (+ more call sites)

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/v3_di_runtime.h"

namespace {
void resetDiCounter(V3DiRuntimeState& runtime) {
  runtime.edgePulse = false;
  runtime.liveValue = 0;
  runtime.startOnMs = 0;
  runtime.startOffMs = 0;
}
}  // namespace

void runV3DiStep(const V3DiRuntimeConfig& cfg, V3DiRuntimeState& runtime,
                 const V3DiStepInput& in, V3DiStepOutput& out) {
  const bool rawSample = in.forceActive ? in.forcedSample : in.sample;
  const bool effectiveSample = cfg.invert ? !rawSample : rawSample;

  out.turnOnConditionMet = in.turnOnCondition;
  out.turnOffConditionMet = in.turnOffCondition;
  out.resetOverride = in.turnOnCondition && in.turnOffCondition;
  out.nextPrevSample = effectiveSample;
  out.nextPrevSampleValid = true;

  // Physical state is polarity-adjusted signal value and is independent from
  // set/reset gating.
  runtime.actualState = effectiveSample;

  if (in.turnOffCondition) {
    // Reset path only clears counter/filter state and inhibits processing.
    resetDiCounter(runtime);
    runtime.state = State_DI_Inhibited;
    return;
  }

  if (!in.turnOnCondition) {
    runtime.edgePulse = false;
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
      runtime.edgePulse = false;
      runtime.state = State_DI_Idle;
      return;
    }

    const uint32_t elapsed = in.nowMs - runtime.startOnMs;
    if (cfg.debounceTimeMs > 0 && elapsed < cfg.debounceTimeMs) {
      runtime.edgePulse = false;
      runtime.state = State_DI_Filtering;
      return;
    }

    runtime.edgePulse = true;
    runtime.liveValue += 1;
    runtime.commandState = effectiveSample;
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
    runtime.edgePulse = false;
    runtime.state = State_DI_Idle;
    return;
  }

  runtime.startOnMs = in.nowMs;
  runtime.startOffMs = effectiveSample ? 1U : 0U;
  if (cfg.debounceTimeMs == 0) {
    runtime.edgePulse = true;
    runtime.liveValue += 1;
    runtime.commandState = effectiveSample;
    runtime.state = State_DI_Qualified;
    return;
  }

  runtime.edgePulse = false;
  runtime.state = State_DI_Filtering;
}



