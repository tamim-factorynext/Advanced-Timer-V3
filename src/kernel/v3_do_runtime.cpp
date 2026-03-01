#include "kernel/v3_do_runtime.h"

namespace {
bool isDoRunningState(cardState state) {
  return state == State_DO_OnDelay || state == State_DO_Active;
}

void forceDoIdle(V3DoRuntimeState& runtime, bool clearCounter) {
  runtime.logicalState = false;
  runtime.physicalState = false;
  runtime.triggerFlag = false;
  runtime.startOnMs = 0;
  runtime.startOffMs = 0;
  runtime.repeatCounter = 0;
  if (clearCounter) {
    runtime.currentValue = 0;
  }
  runtime.state = State_DO_Idle;
}
}  // namespace

void runV3DoStep(const V3DoRuntimeConfig& cfg, V3DoRuntimeState& runtime,
                 const V3DoStepInput& in, V3DoStepOutput& out) {
  out.setResult = in.setCondition;
  out.resetResult = in.resetCondition;
  out.resetOverride = in.setCondition && in.resetCondition;
  out.effectiveOutput = false;

  const bool previousPhysical = runtime.physicalState;

  if (in.resetCondition) {
    forceDoIdle(runtime, true);
    return;
  }

  const bool retriggerable =
      (runtime.state == State_DO_Idle || runtime.state == State_DO_Finished);
  runtime.triggerFlag = retriggerable && in.setCondition;

  if (runtime.triggerFlag) {
    runtime.logicalState = true;
    runtime.repeatCounter = 0;
    if (cfg.mode == Mode_DO_Immediate) {
      runtime.state = State_DO_Active;
      runtime.startOffMs = in.nowMs;
    } else {
      runtime.state = State_DO_OnDelay;
      runtime.startOnMs = in.nowMs;
    }
  }

  if (cfg.mode == Mode_DO_Gated && isDoRunningState(runtime.state) &&
      !in.setCondition) {
    forceDoIdle(runtime, false);
    return;
  }

  bool effectiveOutput = false;
  switch (runtime.state) {
    case State_DO_OnDelay: {
      effectiveOutput = false;
      if (cfg.delayBeforeOnMs == 0) {
        break;
      }
      if ((in.nowMs - runtime.startOnMs) >= cfg.delayBeforeOnMs) {
        runtime.state = State_DO_Active;
        runtime.startOffMs = in.nowMs;
        effectiveOutput = true;
      }
      break;
    }
    case State_DO_Active: {
      effectiveOutput = true;
      if (cfg.onDurationMs == 0) {
        break;
      }
      if ((in.nowMs - runtime.startOffMs) >= cfg.onDurationMs) {
        runtime.repeatCounter += 1;
        effectiveOutput = false;

        if (cfg.repeatCount == 0) {
          runtime.state = State_DO_OnDelay;
          runtime.startOnMs = in.nowMs;
          break;
        }

        if (runtime.repeatCounter >= cfg.repeatCount) {
          runtime.logicalState = false;
          runtime.state = State_DO_Finished;
          break;
        }

        runtime.state = State_DO_OnDelay;
        runtime.startOnMs = in.nowMs;
      }
      break;
    }
    case State_DO_Finished:
    case State_DO_Idle:
    default:
      effectiveOutput = false;
      break;
  }

  if (!previousPhysical && effectiveOutput) {
    runtime.currentValue += 1;
  }

  runtime.physicalState = effectiveOutput;
  out.effectiveOutput = effectiveOutput;
}

