/*
File: src/kernel/legacy_v3_status_runtime.cpp
Purpose: Implements the v3 status runtime module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/kernel_service.cpp
- src/kernel/legacy_v3_status_runtime.h
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#include "kernel/legacy_v3_status_runtime.h"

namespace {
bool isDoStyleMissionType(logicCardType type) {
  return type == DigitalOutput || type == SoftIO;
}
}  // namespace

bool isMissionRunning(logicCardType type, cardState state) {
  if (!isDoStyleMissionType(type)) return false;
  return state == State_DO_OnDelay || state == State_DO_Active;
}

bool isMissionFinished(logicCardType type, cardState state) {
  if (!isDoStyleMissionType(type)) return false;
  return state == State_DO_Finished;
}

bool isMissionStopped(logicCardType type, cardState state) {
  if (!isDoStyleMissionType(type)) return false;
  return state == State_DO_Idle || state == State_DO_Finished;
}



