#pragma once

#include <stdint.h>

#include "control/control_service.h"
#include "control/command_dto.h"
#include "runtime/runtime_service.h"

namespace v3::portal {

enum class PortalCommandType : uint8_t {
  SetRunMode,
  StepOnce,
  SetInputForce,
};

struct PortalCommandSubmitResult {
  bool accepted;
  uint32_t requestId;
  v3::control::CommandRejectReason reason;
};

struct PortalCommandRequest {
  PortalCommandType type;
  runMode mode;
  uint8_t cardId;
  inputSourceMode inputMode;
  uint32_t inputValue;
  uint32_t requestId;
  uint32_t enqueuedUs;
};

struct PortalCommandIngressDiagnostics {
  uint16_t pendingDepth;
  uint16_t pendingHighWater;
  uint32_t requestedCount;
  uint32_t acceptedCount;
  uint32_t rejectedCount;
  uint8_t lastRejectReason;
  bool lastRequestAccepted;
  uint32_t lastRequestId;
  uint32_t queueAcceptedCount;
  uint32_t queueRejectedCount;
};

struct PortalDiagnosticsState {
  bool ready;
  uint32_t revision;
  const char* json;
};

class PortalService {
 public:
  void begin();
  void tick(uint32_t nowMs, const v3::runtime::RuntimeSnapshot& snapshot);
  PortalCommandSubmitResult submitSetRunMode(runMode mode, uint32_t enqueuedUs);
  PortalCommandSubmitResult submitStepOnce(uint32_t enqueuedUs);
  PortalCommandSubmitResult submitSetInputForce(uint8_t cardId,
                                                inputSourceMode inputMode,
                                                uint32_t inputValue,
                                                uint32_t enqueuedUs);
  bool enqueueSetRunModeRequest(runMode mode, uint32_t requestId,
                                uint32_t enqueuedUs);
  bool enqueueStepOnceRequest(uint32_t requestId, uint32_t enqueuedUs);
  bool enqueueSetInputForceRequest(uint8_t cardId, inputSourceMode inputMode,
                                   uint32_t inputValue, uint32_t requestId,
                                   uint32_t enqueuedUs);
  bool dequeueCommandRequest(PortalCommandRequest& out);
  void recordCommandResult(uint32_t requestId, bool accepted,
                           v3::control::CommandRejectReason reason);
  const PortalCommandIngressDiagnostics& commandIngressDiagnostics() const;
  PortalDiagnosticsState diagnosticsState() const;

 private:
  void rebuildDiagnosticsJson(const v3::runtime::RuntimeSnapshot& snapshot);
  bool enqueueRequest(const PortalCommandRequest& request);

  uint32_t lastTickMs_ = 0;
  uint32_t observedScanCount_ = 0;
  uint32_t diagnosticsRevision_ = 0;
  bool diagnosticsReady_ = false;
  char diagnosticsJson_[1024] = {};
  static constexpr uint16_t kPendingCapacity = 16;
  PortalCommandRequest pending_[kPendingCapacity] = {};
  uint16_t head_ = 0;
  uint16_t tail_ = 0;
  uint16_t depth_ = 0;
  uint32_t nextRequestId_ = 0;
  PortalCommandIngressDiagnostics ingress_ = {};
};

}  // namespace v3::portal
