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

/**
 * @brief Portal command intent kinds accepted from transport layer.
 * @details Maps API/WebSocket requests to typed control intents.
 * @par Used By
 * - src/portal/portal_service.cpp
 * - src/main.cpp
 */
enum class PortalCommandType : uint8_t {
  SetRunMode,
  StepOnce,
  SetInputForce,
};

/**
 * @brief Outcome of a portal command submission attempt.
 * @details Couples acceptance flag with request trace id and rejection reason.
 * @par Used By
 * - src/portal/portal_service.cpp
 * - src/portal/transport_command_stub.cpp
 */
struct PortalCommandSubmitResult {
  bool accepted;
  uint32_t requestId;
  v3::control::CommandRejectReason reason;
};

/**
 * @brief Internal queued request format used between portal and control dispatch.
 * @details Holds typed command payload plus tracing metadata.
 * @par Used By
 * - src/portal/portal_service.cpp
 * - src/main.cpp
 */
struct PortalCommandRequest {
  PortalCommandType type;
  engineMode mode;
  uint8_t cardId;
  inputSourceMode inputMode;
  uint32_t inputValue;
  uint32_t requestId;
  uint32_t enqueuedUs;
};

/**
 * @brief Portal ingress diagnostics and queue-pressure counters.
 * @details Exposes acceptance/rejection behavior of portal command buffering stage.
 * @par Used By
 * - src/portal/portal_service.cpp
 * - src/runtime/runtime_service.cpp
 */
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

/**
 * @brief Cached diagnostics JSON state descriptor.
 * @details Indicates readiness and revision of prebuilt diagnostics payload.
 * @par Used By
 * - src/portal/transport_runtime.cpp
 */
struct PortalDiagnosticsState {
  bool ready;
  uint32_t revision;
  const char* json;
};

/**
 * @brief Cached runtime snapshot JSON state descriptor.
 * @details Indicates readiness and revision of prebuilt snapshot payload.
 * @par Used By
 * - src/portal/transport_runtime.cpp
 */
struct PortalSnapshotState {
  bool ready;
  uint32_t revision;
};

/**
 * @brief Owns portal-side command ingress queue and runtime cache projection.
 * @details Decouples transport request handling from control dispatch and lightweight response building.
 * @par Used By
 * - src/main.cpp
 * - src/portal/transport_runtime.cpp
 */
class PortalService {
 public:
  /**
   * @brief Initializes portal state, counters, queue, and JSON reserves.
   * @details Must run before transport handlers begin using portal APIs.
   * @par Used By
   * - src/main.cpp
   */
  void begin();
  /**
   * @brief Rebuilds cached diagnostics payload and runtime delta markers from latest data.
   * @details Called in service loop after runtime snapshot projection update.
   * @param nowMs Current service-loop timestamp.
   * @param snapshot Latest runtime aggregate snapshot.
   * @param cards Per-card runtime snapshot array.
   * @param cardCount Number of valid entries in `cards`.
   * @par Used By
   * - src/main.cpp
   */
  void tick(uint32_t nowMs, const v3::runtime::RuntimeSnapshot& snapshot,
            const RuntimeSnapshotCard* cards, uint8_t cardCount);
  /**
   * @brief Submits run-mode change request into portal pending queue.
   * @details Generates request id and records acceptance/rejection bookkeeping.
   * @par Used By
   * - src/portal/transport_command_stub.cpp
   */
  PortalCommandSubmitResult submitSetRunMode(engineMode mode, uint32_t enqueuedUs);
  /**
   * @brief Submits step-once request into portal pending queue.
   * @details Generates request id and records acceptance/rejection bookkeeping.
   * @par Used By
   * - src/portal/transport_command_stub.cpp
   */
  PortalCommandSubmitResult submitStepOnce(uint32_t enqueuedUs);
  /**
   * @brief Submits input-force request into portal pending queue.
   * @details Generates request id and records acceptance/rejection bookkeeping.
   * @par Used By
   * - src/portal/transport_command_stub.cpp
   */
  PortalCommandSubmitResult submitSetInputForce(uint8_t cardId,
                                                inputSourceMode inputMode,
                                                uint32_t inputValue,
                                                uint32_t enqueuedUs);
  /**
   * @brief Enqueues a prebuilt run-mode request.
   * @details Low-level queue helper used by submit path.
   * @par Used By
   * - src/portal/portal_service.cpp
   */
  bool enqueueSetRunModeRequest(engineMode mode, uint32_t requestId,
                                uint32_t enqueuedUs);
  /**
   * @brief Enqueues a prebuilt step request.
   * @details Low-level queue helper used by submit path.
   * @par Used By
   * - src/portal/portal_service.cpp
   */
  bool enqueueStepOnceRequest(uint32_t requestId, uint32_t enqueuedUs);
  /**
   * @brief Enqueues a prebuilt input-force request.
   * @details Low-level queue helper used by submit path.
   * @par Used By
   * - src/portal/portal_service.cpp
   */
  bool enqueueSetInputForceRequest(uint8_t cardId, inputSourceMode inputMode,
                                   uint32_t inputValue, uint32_t requestId,
                                   uint32_t enqueuedUs);
  /**
   * @brief Pops next portal command request for control dispatch.
   * @details Non-blocking queue pop on portal pending queue.
   * @par Used By
   * - src/main.cpp
   */
  bool dequeueCommandRequest(PortalCommandRequest& out);
  /**
   * @brief Records downstream command accept/reject outcome.
   * @details Updates portal ingress diagnostics after control/kernel queue stage.
   * @par Used By
   * - src/main.cpp
   */
  void recordCommandResult(uint32_t requestId, bool accepted,
                           v3::control::CommandRejectReason reason);
  /**
   * @brief Returns current ingress diagnostics.
   * @details Exposes pending depth, high-water, and accept/reject counters.
   * @par Used By
   * - src/main.cpp
   * - src/runtime/runtime_service.cpp
   */
  const PortalCommandIngressDiagnostics& commandIngressDiagnostics() const;
  /**
   * @brief Returns diagnostics JSON cache state.
   * @details Used by transport endpoint handlers for readiness checks.
   * @par Used By
   * - src/portal/transport_runtime.cpp
   */
  PortalDiagnosticsState diagnosticsState() const;
  /**
   * @brief Returns runtime snapshot JSON cache state.
   * @details Used by transport endpoint handlers for readiness checks.
   * @par Used By
   * - src/portal/transport_runtime.cpp
   */
  PortalSnapshotState snapshotState() const;
  /**
   * @brief Returns latest structured runtime snapshot captured during portal tick.
   * @details Enables transport handlers to project lightweight responses
   * without reparsing cached snapshot JSON.
   * @return Latest runtime snapshot value copy.
   */
  const v3::runtime::RuntimeSnapshot& latestRuntimeSnapshot() const;
  /**
   * @brief Returns latest structured per-card runtime rows.
   * @param outCardCount Receives number of valid card rows.
   * @return Pointer to internal card cache.
   */
  const RuntimeSnapshotCard* latestRuntimeCards(uint8_t& outCardCount) const;
  /**
   * @brief Returns previous runtime snapshot sequence observed before current one.
   * @return Previous completed-scan sequence.
   */
  uint32_t previousRuntimeSnapshotSeq() const;
  /**
   * @brief Indicates whether runtime card row changed from previous sequence.
   * @param index Card row index in latest runtime card slice.
   * @return `true` when row differs from previous snapshot row.
   */
  bool isRuntimeCardChangedSincePrevious(uint8_t index) const;

 private:
  static constexpr size_t kDiagnosticsJsonReserve = 1024;

  void rebuildDiagnosticsJson(const v3::runtime::RuntimeSnapshot& snapshot);
  bool enqueueRequest(const PortalCommandRequest& request);
  static uint32_t cardSignature(const RuntimeSnapshotCard& card);

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
  v3::runtime::RuntimeSnapshot latestRuntimeSnapshot_ = {};
  const RuntimeSnapshotCard* latestRuntimeCardsPtr_ = nullptr;
  uint8_t latestRuntimeCardCount_ = 0;
  uint32_t latestRuntimeSnapshotSeq_ = 0;
  uint32_t previousRuntimeSnapshotSeq_ = 0;
  uint8_t previousRuntimeCardCount_ = 0;
  uint8_t latestCardIds_[v3::storage::kMaxCards] = {};
  uint8_t previousCardIds_[v3::storage::kMaxCards] = {};
  uint32_t latestCardSignatures_[v3::storage::kMaxCards] = {};
  uint32_t previousCardSignatures_[v3::storage::kMaxCards] = {};
  static constexpr uint16_t kPendingCapacity = 16;
  PortalCommandRequest pending_[kPendingCapacity] = {};
  uint16_t head_ = 0;
  uint16_t tail_ = 0;
  uint16_t depth_ = 0;
  uint32_t nextRequestId_ = 0;
  PortalCommandIngressDiagnostics ingress_ = {};
};

}  // namespace v3::portal

