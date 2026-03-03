#include "kernel/v3_sio_runtime.h"

void runV3SioStep(const V3SioRuntimeConfig& cfg, V3SioRuntimeState& runtime,
                  const V3SioStepInput& in, V3SioStepOutput& out) {
  V3DoRuntimeConfig doCfg = {};
  doCfg.mode = cfg.mode;
  doCfg.invert = cfg.invert;
  doCfg.delayBeforeOnMs = cfg.delayBeforeOnMs;
  doCfg.activeDurationMs = cfg.activeDurationMs;
  doCfg.repeatCount = cfg.repeatCount;

  V3DoRuntimeState doState = {};
  doState.commandState = runtime.commandState;
  doState.actualState = runtime.actualState;
  doState.edgePulse = runtime.edgePulse;
  doState.liveValue = runtime.liveValue;
  doState.startOnMs = runtime.startOnMs;
  doState.startOffMs = runtime.startOffMs;
  doState.repeatCounter = runtime.repeatCounter;
  doState.state = runtime.state;

  V3DoStepInput doIn = {};
  doIn.nowMs = in.nowMs;
  doIn.turnOnCondition = in.turnOnCondition;
  doIn.turnOffCondition = in.turnOffCondition;

  V3DoStepOutput doOut = {};
  runV3DoStep(doCfg, doState, doIn, doOut);

  runtime.commandState = doState.commandState;
  runtime.actualState = doState.actualState;
  runtime.edgePulse = doState.edgePulse;
  runtime.liveValue = doState.liveValue;
  runtime.startOnMs = doState.startOnMs;
  runtime.startOffMs = doState.startOffMs;
  runtime.repeatCounter = doState.repeatCounter;
  runtime.state = doState.state;

  out.turnOnConditionMet = doOut.turnOnConditionMet;
  out.turnOffConditionMet = doOut.turnOffConditionMet;
  out.resetOverride = doOut.resetOverride;
  out.effectiveOutput = doOut.effectiveOutput;
}



