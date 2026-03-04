/*
File: src/control/control_service.cpp
Purpose: Implements the control service module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/control/control_service.h
- src/main.cpp
- src/portal/portal_service.h
Flow Hook:
- Control command orchestration and system-level actions.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#include "control/control_service.h"

namespace v3::control {

/**
 * @brief Resets control queue state and diagnostics counters.
 * @par Used By
 * main startup initialization.
 */
void ControlService::begin() {
  lastTickMs_ = 0;
  desiredMode_ = RUN_NORMAL;
  head_ = 0;
  tail_ = 0;
  depth_ = 0;
  diagnostics_ = {};
  diagnostics_.lastRejectReason = CommandRejectReason::None;
}

/**
 * @brief Advances control service heartbeat timestamp.
 * @param nowMs Current monotonic tick time in milliseconds.
 * @par Used By
 * Main loop tick.
 */
void ControlService::tick(uint32_t nowMs) { lastTickMs_ = nowMs; }

/**
 * @brief Enqueues a run-mode transition request.
 * @param mode Requested engine run mode.
 * @param nowUs Enqueue timestamp in microseconds.
 * @param requestId Correlation id supplied by portal.
 * @retval true Request accepted and queued.
 * @retval false Request rejected due to validation/queue capacity.
 * @par Used By
 * Portal command dispatch.
 */
bool ControlService::requestSetRunMode(engineMode mode, uint32_t nowUs,
                                       uint32_t requestId) {
  diagnostics_.requestedCount += 1;

  if (!isValidRunMode(mode)) {
    diagnostics_.rejectedCount += 1;
    diagnostics_.lastRejectReason = CommandRejectReason::InvalidRunMode;
    return false;
  }

  KernelCommand command = {};
  command.type = KernelCmd_SetRunMode;
  command.mode = mode;
  command.enqueuedUs = nowUs;
  command.requestId = requestId;

  if (!enqueueCommand(command)) {
    diagnostics_.rejectedCount += 1;
    diagnostics_.lastRejectReason = CommandRejectReason::QueueFull;
    return false;
  }

  desiredMode_ = mode;
  diagnostics_.acceptedCount += 1;
  diagnostics_.lastRejectReason = CommandRejectReason::None;
  return true;
}

/**
 * @brief Enqueues one step command when run mode allows stepping.
 * @param nowUs Enqueue timestamp in microseconds.
 * @param requestId Correlation id supplied by portal.
 * @retval true Request accepted and queued.
 * @retval false Request rejected (wrong mode or queue full).
 * @par Used By
 * Portal command dispatch.
 */
bool ControlService::requestStepOnce(uint32_t nowUs, uint32_t requestId) {
  diagnostics_.requestedCount += 1;

  if (desiredMode_ != RUN_STEP) {
    diagnostics_.rejectedCount += 1;
    diagnostics_.lastRejectReason = CommandRejectReason::StepRequiresRunStep;
    return false;
  }

  KernelCommand command = {};
  command.type = KernelCmd_StepOnce;
  command.enqueuedUs = nowUs;
  command.requestId = requestId;

  if (!enqueueCommand(command)) {
    diagnostics_.rejectedCount += 1;
    diagnostics_.lastRejectReason = CommandRejectReason::QueueFull;
    return false;
  }

  diagnostics_.acceptedCount += 1;
  diagnostics_.lastRejectReason = CommandRejectReason::None;
  return true;
}

/**
 * @brief Enqueues DI/AI input force command for one card.
 * @param cardId Target card id.
 * @param inputMode Requested input source mode.
 * @param nowUs Enqueue timestamp in microseconds.
 * @param requestId Correlation id supplied by portal.
 * @param forcedValue Optional forced numeric value for AI force mode.
 * @retval true Request accepted and queued.
 * @retval false Request rejected due to invalid mode or queue full.
 * @par Used By
 * Portal command dispatch.
 */
bool ControlService::requestSetInputForce(uint8_t cardId, inputSourceMode inputMode,
                                          uint32_t nowUs, uint32_t requestId,
                                          uint32_t forcedValue) {
  diagnostics_.requestedCount += 1;

  if (inputMode != InputSource_Real && inputMode != InputSource_ForcedHigh &&
      inputMode != InputSource_ForcedLow &&
      inputMode != InputSource_ForcedValue) {
    diagnostics_.rejectedCount += 1;
    diagnostics_.lastRejectReason = CommandRejectReason::InvalidInputMode;
    return false;
  }

  KernelCommand command = {};
  command.type = KernelCmd_SetInputForce;
  command.cardId = cardId;
  command.inputMode = inputMode;
  command.enqueuedUs = nowUs;
  command.requestId = requestId;
  command.value = forcedValue;

  if (!enqueueCommand(command)) {
    diagnostics_.rejectedCount += 1;
    diagnostics_.lastRejectReason = CommandRejectReason::QueueFull;
    return false;
  }

  diagnostics_.acceptedCount += 1;
  diagnostics_.lastRejectReason = CommandRejectReason::None;
  return true;
}

/**
 * @brief Pops next pending command for kernel dispatch.
 * @param out Destination command.
 * @retval true Command dequeued.
 * @retval false Queue empty.
 * @par Used By
 * Main loop command forwarding.
 */
bool ControlService::dequeueCommand(KernelCommand& out) {
  if (depth_ == 0) return false;

  out = pending_[head_];
  head_ = static_cast<uint16_t>((head_ + 1) % kPendingCapacity);
  depth_ -= 1;
  diagnostics_.pendingDepth = depth_;
  return true;
}

/**
 * @brief Returns current control diagnostics counters.
 * @return Immutable diagnostics view.
 */
const ControlDiagnostics& ControlService::diagnostics() const {
  return diagnostics_;
}

/**
 * @brief Pushes command into bounded pending queue.
 * @param command Command to store.
 * @retval true Command queued.
 * @retval false Queue full.
 * @par Used By
 * requestSetRunMode(), requestStepOnce(), requestSetInputForce().
 */
bool ControlService::enqueueCommand(const KernelCommand& command) {
  if (depth_ >= kPendingCapacity) return false;

  pending_[tail_] = command;
  tail_ = static_cast<uint16_t>((tail_ + 1) % kPendingCapacity);
  depth_ += 1;
  diagnostics_.pendingDepth = depth_;
  if (depth_ > diagnostics_.pendingHighWater) {
    diagnostics_.pendingHighWater = depth_;
  }
  return true;
}

/**
 * @brief Validates run-mode enum value.
 * @param mode Requested mode.
 * @retval true Mode is one of supported run modes.
 * @retval false Mode value unsupported.
 * @par Used By
 * requestSetRunMode().
 */
bool ControlService::isValidRunMode(engineMode mode) {
  switch (mode) {
    case RUN_NORMAL:
    case RUN_STEP:
    case RUN_BREAKPOINT:
      return true;
    default:
      return false;
  }
}

}  // namespace v3::control


