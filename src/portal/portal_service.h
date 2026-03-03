/*
File: src/portal/portal_service.h
Purpose: Declares the portal service module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/main.cpp
- src/portal/transport_runtime.h
- src/portal/transport_command_stub.h
Flow Hook:
- Portal request handling and runtime snapshot transport.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <WString.h>

#include "control/control_service.h"
#include "control/command_dto.h"
#include "runtime/runtime_snapshot_card.h"
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
  engineMode mode;
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

struct PortalSnapshotState {
  bool ready;
  uint32_t revision;
  const char* json;
};

class PortalService {
 public:
  void begin();
  void tick(uint32_t nowMs, const v3::runtime::RuntimeSnapshot& snapshot,
            const RuntimeSnapshotCard* cards, uint8_t cardCount);
  PortalCommandSubmitResult submitSetRunMode(engineMode mode, uint32_t enqueuedUs);
  PortalCommandSubmitResult submitStepOnce(uint32_t enqueuedUs);
  PortalCommandSubmitResult submitSetInputForce(uint8_t cardId,
                                                inputSourceMode inputMode,
                                                uint32_t inputValue,
                                                uint32_t enqueuedUs);
  bool enqueueSetRunModeRequest(engineMode mode, uint32_t requestId,
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
  PortalSnapshotState snapshotState() const;

 private:
  static constexpr size_t kDiagnosticsJsonReserve = 1024;
  static constexpr size_t kSnapshotJsonReserve = 12288;

  void rebuildDiagnosticsJson(const v3::runtime::RuntimeSnapshot& snapshot);
  void rebuildSnapshotJson(const v3::runtime::RuntimeSnapshot& snapshot,
                           const RuntimeSnapshotCard* cards, uint8_t cardCount);
  bool enqueueRequest(const PortalCommandRequest& request);

  uint32_t lastTickMs_ = 0;
  uint32_t observedScanCount_ = 0;
  uint32_t diagnosticsRevision_ = 0;
  bool diagnosticsReady_ = false;
  bool diagnosticsFallbackActive_ = false;
  bool diagnosticsReserveReady_ = false;
  uint32_t diagnosticsSerializeFailureCount_ = 0;
  uint32_t diagnosticsCapacityRejectCount_ = 0;
  String diagnosticsJson_;
  uint32_t snapshotRevision_ = 0;
  bool snapshotReady_ = false;
  bool snapshotFallbackActive_ = false;
  bool snapshotReserveReady_ = false;
  uint32_t snapshotSerializeFailureCount_ = 0;
  uint32_t snapshotCapacityRejectCount_ = 0;
  String snapshotJson_;
  static constexpr uint16_t kPendingCapacity = 16;
  PortalCommandRequest pending_[kPendingCapacity] = {};
  uint16_t head_ = 0;
  uint16_t tail_ = 0;
  uint16_t depth_ = 0;
  uint32_t nextRequestId_ = 0;
  PortalCommandIngressDiagnostics ingress_ = {};
};

}  // namespace v3::portal

