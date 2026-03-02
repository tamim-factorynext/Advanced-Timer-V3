#pragma once

#include <stdint.h>

#include "control/command_dto.h"

namespace v3::control {

enum class CommandRejectReason : uint8_t {
  None,
  QueueFull,
  InvalidRunMode,
  InvalidInputMode,
  StepRequiresRunStep,
};

struct ControlDiagnostics {
  uint16_t pendingDepth;
  uint16_t pendingHighWater;
  uint32_t requestedCount;
  uint32_t acceptedCount;
  uint32_t rejectedCount;
  CommandRejectReason lastRejectReason;
};

class ControlService {
 public:
  void begin();
  void tick(uint32_t nowMs);
  bool requestSetRunMode(runMode mode, uint32_t nowUs, uint32_t requestId = 0);
  bool requestStepOnce(uint32_t nowUs, uint32_t requestId = 0);
  bool requestSetInputForce(uint8_t cardId, inputSourceMode inputMode,
                            uint32_t nowUs, uint32_t requestId = 0);
  bool dequeueCommand(KernelCommand& out);
  const ControlDiagnostics& diagnostics() const;

 private:
  bool enqueueCommand(const KernelCommand& command);
  static bool isValidRunMode(runMode mode);

  static constexpr uint16_t kPendingCapacity = 16;
  KernelCommand pending_[kPendingCapacity] = {};
  uint16_t head_ = 0;
  uint16_t tail_ = 0;
  uint16_t depth_ = 0;
  uint32_t lastTickMs_ = 0;
  runMode desiredMode_ = RUN_NORMAL;
  ControlDiagnostics diagnostics_ = {};
};

}  // namespace v3::control
