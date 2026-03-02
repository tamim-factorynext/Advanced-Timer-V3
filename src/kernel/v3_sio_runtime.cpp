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
  doState.logicalState = runtime.logicalState;
  doState.physicalState = runtime.physicalState;
  doState.triggerFlag = runtime.triggerFlag;
  doState.currentValue = runtime.currentValue;
  doState.startOnMs = runtime.startOnMs;
  doState.startOffMs = runtime.startOffMs;
  doState.repeatCounter = runtime.repeatCounter;
  doState.state = runtime.state;

  V3DoStepInput doIn = {};
  doIn.nowMs = in.nowMs;
  doIn.setCondition = in.setCondition;
  doIn.resetCondition = in.resetCondition;

  V3DoStepOutput doOut = {};
  runV3DoStep(doCfg, doState, doIn, doOut);

  runtime.logicalState = doState.logicalState;
  runtime.physicalState = doState.physicalState;
  runtime.triggerFlag = doState.triggerFlag;
  runtime.currentValue = doState.currentValue;
  runtime.startOnMs = doState.startOnMs;
  runtime.startOffMs = doState.startOffMs;
  runtime.repeatCounter = doState.repeatCounter;
  runtime.state = doState.state;

  out.setResult = doOut.setResult;
  out.resetResult = doOut.resetResult;
  out.resetOverride = doOut.resetOverride;
  out.effectiveOutput = doOut.effectiveOutput;
}
