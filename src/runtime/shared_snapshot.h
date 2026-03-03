/*
File: src/runtime/shared_snapshot.h
Purpose: Declares the shared snapshot module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src\runtime\shared_snapshot.h

Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "control/command_dto.h"
#include "runtime/runtime_snapshot_card.h"

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
  engineMode mode;
  bool testModeActive;
  bool globalOutputMask;
  bool breakpointPaused;
  uint16_t scanCursor;
  RuntimeSnapshotCard cards[N];
  inputSourceMode inputSource[N];
  uint32_t forcedAIValue[N];
  bool outputMaskLocal[N];
  bool breakpointEnabled[N];
  bool turnOnConditionMet[N];
  bool turnOffConditionMet[N];
  uint32_t evalCounter[N];
};



