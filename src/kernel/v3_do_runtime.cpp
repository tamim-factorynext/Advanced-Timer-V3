#include "kernel/v3_do_runtime.h"

namespace {
bool isDoRunningState(cardState state) {
  return state == State_DO_OnDelay || state == State_DO_Active;
}

void forceDoIdle(V3DoRuntimeState& runtime, bool clearCounter) {
  runtime.logicalState = false;
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
  const bool idlePhysical = cfg.invert;

  if (in.resetCondition) {
    forceDoIdle(runtime, true);
    runtime.physicalState = idlePhysical;
    out.effectiveOutput = runtime.physicalState;
    return;
  }

  const bool retriggerable =
      (runtime.state == State_DO_Idle || runtime.state == State_DO_Finished);
  const bool startMission = retriggerable && in.setCondition;
  runtime.triggerFlag = false;

  if (startMission) {
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
    runtime.physicalState = idlePhysical;
    out.effectiveOutput = runtime.physicalState;
    return;
  }

  bool missionOutput = false;
  switch (runtime.state) {
    case State_DO_OnDelay: {
      missionOutput = false;
      if (cfg.delayBeforeOnMs == 0) {
        runtime.state = State_DO_Active;
        runtime.startOffMs = in.nowMs;
        missionOutput = true;
        break;
      }
      if ((in.nowMs - runtime.startOnMs) >= cfg.delayBeforeOnMs) {
        runtime.state = State_DO_Active;
        runtime.startOffMs = in.nowMs;
        missionOutput = true;
      }
      break;
    }
    case State_DO_Active: {
      missionOutput = true;
      if (cfg.activeDurationMs == 0) {
        break;
      }
      if ((in.nowMs - runtime.startOffMs) >= cfg.activeDurationMs) {
        runtime.repeatCounter += 1;
        missionOutput = false;

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
      missionOutput = false;
      break;
  }

  const bool effectiveOutput = cfg.invert ? !missionOutput : missionOutput;
  const bool risingPhysical = (!previousPhysical && effectiveOutput);
  if (risingPhysical) {
    runtime.currentValue += 1;
  }
  runtime.triggerFlag = risingPhysical;

  runtime.physicalState = effectiveOutput;
  out.effectiveOutput = effectiveOutput;
}
