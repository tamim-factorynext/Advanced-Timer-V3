#include "kernel/v3_status_runtime.h"

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

