#pragma once

#include <stddef.h>
#include <stdint.h>

#include "control/command_dto.h"
#include "kernel/card_model.h"

template <size_t N>
struct SharedRuntimeSnapshotT {
  uint32_t seq;
  uint32_t tsMs;
  uint32_t lastCompleteScanUs;
  uint32_t maxCompleteScanUs;
  uint32_t scanBudgetUs;
  uint32_t scanOverrunCount;
  bool scanOverrunLast;
  uint16_t kernelQueueDepth;
  uint16_t kernelQueueHighWaterMark;
  uint16_t kernelQueueCapacity;
  uint32_t commandLatencyLastUs;
  uint32_t commandLatencyMaxUs;
  uint32_t rtcMinuteTickCount;
  uint32_t rtcIntentEnqueueCount;
  uint32_t rtcIntentEnqueueFailCount;
  uint32_t rtcLastEvalMs;
  runMode mode;
  bool testModeActive;
  bool globalOutputMask;
  bool breakpointPaused;
  uint16_t scanCursor;
  LogicCard cards[N];
  inputSourceMode inputSource[N];
  uint32_t forcedAIValue[N];
  bool outputMaskLocal[N];
  bool breakpointEnabled[N];
  bool setResult[N];
  bool resetResult[N];
  bool resetOverride[N];
  uint32_t evalCounter[N];
};
