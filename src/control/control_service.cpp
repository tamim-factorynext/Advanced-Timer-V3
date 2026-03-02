#include "control/control_service.h"

namespace v3::control {

void ControlService::begin() {
  lastTickMs_ = 0;
  desiredMode_ = RUN_NORMAL;
  head_ = 0;
  tail_ = 0;
  depth_ = 0;
  diagnostics_ = {};
  diagnostics_.lastRejectReason = CommandRejectReason::None;
}

void ControlService::tick(uint32_t nowMs) { lastTickMs_ = nowMs; }

bool ControlService::requestSetRunMode(runMode mode, uint32_t nowUs,
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
  command.value = requestId;

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
  command.value = requestId;

  if (!enqueueCommand(command)) {
    diagnostics_.rejectedCount += 1;
    diagnostics_.lastRejectReason = CommandRejectReason::QueueFull;
    return false;
  }

  diagnostics_.acceptedCount += 1;
  diagnostics_.lastRejectReason = CommandRejectReason::None;
  return true;
}

bool ControlService::dequeueCommand(KernelCommand& out) {
  if (depth_ == 0) return false;

  out = pending_[head_];
  head_ = static_cast<uint16_t>((head_ + 1) % kPendingCapacity);
  depth_ -= 1;
  diagnostics_.pendingDepth = depth_;
  return true;
}

const ControlDiagnostics& ControlService::diagnostics() const {
  return diagnostics_;
}

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

bool ControlService::isValidRunMode(runMode mode) {
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
