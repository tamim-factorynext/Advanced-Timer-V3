#include "kernel/v3_di_runtime.h"

namespace {
void resetDiRuntime(V3DiRuntimeState& runtime) {
  runtime.logicalState = false;
  runtime.triggerFlag = false;
  runtime.currentValue = 0;
  runtime.startOnMs = 0;
  runtime.startOffMs = 0;
  runtime.repeatCounter = 0;
}
}  // namespace

void runV3DiStep(const V3DiRuntimeConfig& cfg, V3DiRuntimeState& runtime,
                 const V3DiStepInput& in, V3DiStepOutput& out) {
  out.setResult = in.setCondition;
  out.resetResult = in.resetCondition;
  out.resetOverride = in.setCondition && in.resetCondition;
  out.nextPrevSample = in.sample;
  out.nextPrevSampleValid = true;

  runtime.physicalState = in.sample;

  if (in.resetCondition) {
    resetDiRuntime(runtime);
    runtime.state = State_DI_Inhibited;
    return;
  }

  if (!in.setCondition) {
    runtime.triggerFlag = false;
    runtime.state = State_DI_Idle;
    return;
  }

  bool previousSample = in.sample;
  if (in.prevSampleValid) {
    previousSample = in.prevSample;
  }

  const bool risingEdge = (!previousSample && in.sample);
  const bool fallingEdge = (previousSample && !in.sample);
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

  const uint32_t elapsed = in.nowMs - runtime.startOnMs;
  if (cfg.debounceTimeMs > 0 && elapsed < cfg.debounceTimeMs) {
    runtime.triggerFlag = false;
    runtime.state = State_DI_Filtering;
    return;
  }

  runtime.triggerFlag = true;
  runtime.currentValue += 1;
  runtime.logicalState = in.sample;
  runtime.startOnMs = in.nowMs;
  runtime.state = State_DI_Qualified;
}
