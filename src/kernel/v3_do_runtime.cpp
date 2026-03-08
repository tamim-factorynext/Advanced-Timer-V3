/*
File: src/kernel/v3_do_runtime.cpp
Purpose: Implements the v3 do runtime module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/kernel_service.h
- src/kernel/v3_do_runtime.h
- src/kernel/v3_runtime_adapters.h
- (+ more call sites)
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/v3_do_runtime.h"

namespace {
constexpr uint32_t kCounterWrapMax = 1000000U;
/**
 * @brief Checks whether DO runtime state is in an active mission phase.
 * @details Running phases are `OnDelay` and `Active`.
 * @par Used By
 * - src/kernel/v3_do_runtime.cpp
 */
bool isDoRunningState(cardState state) {
  return state == State_DO_OnDelay || state == State_DO_Active;
}

/** @brief Increments counter with explicit business-rule wrap-to-zero. */
void incrementWrapU32(uint32_t& value) {
  value = (value >= kCounterWrapMax) ? 0U : (value + 1U);
}

/**
 * @brief Forces DO runtime into idle state.
 * @details Clears timing and edge fields; optionally clears output counter value.
 * @par Used By
 * - src/kernel/v3_do_runtime.cpp
 */
void forceDoIdle(V3DoRuntimeState& runtime, bool clearCounter) {
  runtime.commandState = false;
  runtime.edgePulse = false;
  runtime.startOnMs = 0;
  runtime.startOffMs = 0;
  runtime.repeatCounter = 0;
  if (clearCounter) {
    runtime.liveValue = 0;
  }
  runtime.state = State_DO_Idle;
}
}  // namespace

/**
 * @brief Executes one DO card state-machine step.
 * @details Handles reset override, retrigger behavior, timing windows, and inversion mapping.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 * - src/kernel/v3_sio_runtime.cpp
 */
void runV3DoStep(const V3DoRuntimeConfig& cfg, V3DoRuntimeState& runtime,
                 const V3DoStepInput& in, V3DoStepOutput& out) {
  out.turnOnConditionMet = in.turnOnCondition;
  out.turnOffConditionMet = in.turnOffCondition;
  out.resetOverride = in.turnOnCondition && in.turnOffCondition;
  out.effectiveOutput = false;

  const bool previousPhysical = runtime.actualState;
  const bool idlePhysical = cfg.invert;

  if (in.turnOffCondition) {
    forceDoIdle(runtime, true);
    runtime.actualState = idlePhysical;
    out.effectiveOutput = runtime.actualState;
    return;
  }

  const bool retriggerable =
      (runtime.state == State_DO_Idle || runtime.state == State_DO_Finished);
  const bool startMission = retriggerable && in.turnOnCondition;
  runtime.edgePulse = false;

  if (startMission) {
    runtime.commandState = true;
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
      !in.turnOnCondition) {
    forceDoIdle(runtime, false);
    runtime.actualState = idlePhysical;
    out.effectiveOutput = runtime.actualState;
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
          runtime.commandState = false;
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
    incrementWrapU32(runtime.liveValue);
  }
  runtime.edgePulse = risingPhysical;

  runtime.actualState = effectiveOutput;
  out.effectiveOutput = effectiveOutput;
}



