/*
File: src/portal/portal_service.cpp
Purpose: Implements the portal service module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/main.cpp
- src/portal/transport_runtime.cpp
- src/portal/transport_command_stub.cpp
Flow Hook:
- Portal request handling and runtime snapshot transport.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "portal/portal_service.h"

#include <ArduinoJson.h>

#include "kernel/enum_codec.h"

namespace v3::portal {

namespace {

const char* kDiagnosticsFallbackJson =
    "{\"ok\":false,\"reason\":\"diagnostics_memory_pressure\"}";
const char* kSnapshotFallbackJson =
    "{\"ok\":false,\"reason\":\"snapshot_memory_pressure\"}";

}  // namespace

void PortalService::begin() {
  lastTickMs_ = 0;
  observedScanCount_ = 0;
  diagnosticsRevision_ = 0;
  diagnosticsReady_ = false;
  diagnosticsFallbackActive_ = false;
  diagnosticsSerializeFailureCount_ = 0;
  diagnosticsCapacityRejectCount_ = 0;
  diagnosticsJson_.remove(0);
  diagnosticsReserveReady_ = diagnosticsJson_.reserve(kDiagnosticsJsonReserve);
  snapshotRevision_ = 0;
  snapshotReady_ = false;
  snapshotFallbackActive_ = false;
  snapshotSerializeFailureCount_ = 0;
  snapshotCapacityRejectCount_ = 0;
  snapshotJson_.remove(0);
  snapshotReserveReady_ = snapshotJson_.reserve(kSnapshotJsonReserve);
  latestRuntimeSnapshot_ = {};
  latestRuntimeCardsPtr_ = nullptr;
  latestRuntimeCardCount_ = 0;
  head_ = 0;
  tail_ = 0;
  depth_ = 0;
  nextRequestId_ = 0;
  ingress_ = {};
}

void PortalService::tick(uint32_t nowMs,
                         const v3::runtime::RuntimeSnapshot& snapshot,
                         const RuntimeSnapshotCard* cards, uint8_t cardCount) {
  lastTickMs_ = nowMs;
  observedScanCount_ = snapshot.completedScans;
  latestRuntimeSnapshot_ = snapshot;
  if (cards == nullptr) {
    latestRuntimeCardsPtr_ = nullptr;
    latestRuntimeCardCount_ = 0;
  } else {
    latestRuntimeCardsPtr_ = cards;
    latestRuntimeCardCount_ = (cardCount <= v3::storage::kMaxCards)
                                  ? cardCount
                                  : v3::storage::kMaxCards;
  }
  rebuildDiagnosticsJson(snapshot);
  rebuildSnapshotJson(snapshot, cards, cardCount);
}

bool PortalService::enqueueSetRunModeRequest(engineMode mode, uint32_t requestId,
                                             uint32_t enqueuedUs) {
  PortalCommandRequest request = {};
  request.type = PortalCommandType::SetRunMode;
  request.mode = mode;
  request.requestId = requestId;
  request.enqueuedUs = enqueuedUs;
  return enqueueRequest(request);
}

bool PortalService::enqueueStepOnceRequest(uint32_t requestId,
                                           uint32_t enqueuedUs) {
  PortalCommandRequest request = {};
  request.type = PortalCommandType::StepOnce;
  request.mode = RUN_STEP;
  request.requestId = requestId;
  request.enqueuedUs = enqueuedUs;
  return enqueueRequest(request);
}

bool PortalService::enqueueSetInputForceRequest(uint8_t cardId,
                                                inputSourceMode inputMode,
                                                uint32_t inputValue,
                                                uint32_t requestId,
                                                uint32_t enqueuedUs) {
  PortalCommandRequest request = {};
  request.type = PortalCommandType::SetInputForce;
  request.mode = RUN_NORMAL;
  request.cardId = cardId;
  request.inputMode = inputMode;
  request.inputValue = inputValue;
  request.requestId = requestId;
  request.enqueuedUs = enqueuedUs;
  return enqueueRequest(request);
}

PortalCommandSubmitResult PortalService::submitSetRunMode(engineMode mode,
                                                          uint32_t enqueuedUs) {
  PortalCommandSubmitResult result = {};
  result.requestId = ++nextRequestId_;
  result.reason = v3::control::CommandRejectReason::None;
  result.accepted = enqueueSetRunModeRequest(mode, result.requestId, enqueuedUs);
  if (!result.accepted) {
    result.reason = v3::control::CommandRejectReason::QueueFull;
  }
  return result;
}

PortalCommandSubmitResult PortalService::submitStepOnce(uint32_t enqueuedUs) {
  PortalCommandSubmitResult result = {};
  result.requestId = ++nextRequestId_;
  result.reason = v3::control::CommandRejectReason::None;
  result.accepted = enqueueStepOnceRequest(result.requestId, enqueuedUs);
  if (!result.accepted) {
    result.reason = v3::control::CommandRejectReason::QueueFull;
  }
  return result;
}

PortalCommandSubmitResult PortalService::submitSetInputForce(
    uint8_t cardId, inputSourceMode inputMode, uint32_t inputValue,
    uint32_t enqueuedUs) {
  PortalCommandSubmitResult result = {};
  result.requestId = ++nextRequestId_;
  result.reason = v3::control::CommandRejectReason::None;
  result.accepted = enqueueSetInputForceRequest(cardId, inputMode, inputValue,
                                                result.requestId, enqueuedUs);
  if (!result.accepted) {
    result.reason = v3::control::CommandRejectReason::QueueFull;
  }
  return result;
}

bool PortalService::dequeueCommandRequest(PortalCommandRequest& out) {
  if (depth_ == 0) return false;
  out = pending_[head_];
  head_ = static_cast<uint16_t>((head_ + 1) % kPendingCapacity);
  depth_ -= 1;
  ingress_.pendingDepth = depth_;
  return true;
}

void PortalService::recordCommandResult(uint32_t requestId, bool accepted,
                                        v3::control::CommandRejectReason reason) {
  ingress_.lastRequestId = requestId;
  ingress_.lastRequestAccepted = accepted;
  ingress_.lastRejectReason = static_cast<uint8_t>(reason);
  if (accepted) {
    ingress_.acceptedCount += 1;
  } else {
    ingress_.rejectedCount += 1;
  }
}

const PortalCommandIngressDiagnostics& PortalService::commandIngressDiagnostics()
    const {
  return ingress_;
}

PortalDiagnosticsState PortalService::diagnosticsState() const {
  PortalDiagnosticsState state = {};
  state.ready = diagnosticsReady_;
  state.revision = diagnosticsRevision_;
  state.json =
      diagnosticsFallbackActive_ ? kDiagnosticsFallbackJson : diagnosticsJson_.c_str();
  return state;
}

PortalSnapshotState PortalService::snapshotState() const {
  PortalSnapshotState state = {};
  state.ready = snapshotReady_;
  state.revision = snapshotRevision_;
  state.json = snapshotFallbackActive_ ? kSnapshotFallbackJson : snapshotJson_.c_str();
  return state;
}

const v3::runtime::RuntimeSnapshot& PortalService::latestRuntimeSnapshot() const {
  return latestRuntimeSnapshot_;
}

const RuntimeSnapshotCard* PortalService::latestRuntimeCards(
    uint8_t& outCardCount) const {
  outCardCount = latestRuntimeCardCount_;
  return latestRuntimeCardsPtr_;
}

void PortalService::rebuildDiagnosticsJson(
    const v3::runtime::RuntimeSnapshot& snapshot) {
  JsonDocument doc;

  doc["revision"] = diagnosticsRevision_ + 1;
  doc["nowMs"] = snapshot.nowMs;

  JsonObject binding = doc["binding"].to<JsonObject>();
  binding["configuredCardCount"] = snapshot.configuredCardCount;
  binding["enabledCardCount"] = snapshot.enabledCardCount;
  binding["familyCountSum"] = snapshot.familyCountSum;
  binding["consistent"] = snapshot.bindingConsistent;

  JsonObject family = binding["families"].to<JsonObject>();
  family["di"] = snapshot.diCardCount;
  family["do"] = snapshot.doCardCount;
  family["ai"] = snapshot.aiCardCount;
  family["sio"] = snapshot.sioCardCount;
  family["math"] = snapshot.mathCardCount;
  family["rtc"] = snapshot.rtcCardCount;

  JsonObject diRuntime = binding["diRuntime"].to<JsonObject>();
  diRuntime["totalQualifiedEdges"] = snapshot.diTotalQualifiedEdges;
  diRuntime["inhibitedCount"] = snapshot.diInhibitedCount;
  diRuntime["forcedCount"] = snapshot.diForcedCount;

  JsonObject aiRuntime = binding["aiRuntime"].to<JsonObject>();
  aiRuntime["forcedCount"] = snapshot.aiForcedCount;

  JsonObject doRuntime = binding["doRuntime"].to<JsonObject>();
  doRuntime["activeCount"] = snapshot.doActiveCount;

  JsonObject sioRuntime = binding["sioRuntime"].to<JsonObject>();
  sioRuntime["activeCount"] = snapshot.sioActiveCount;

  JsonObject bootstrap = doc["bootstrap"].to<JsonObject>();
  bootstrap["usedFileConfig"] = snapshot.bootstrapUsedFileConfig;
  bootstrap["hasActiveConfig"] = snapshot.storageHasActiveConfig;
  bootstrap["errorCode"] =
      v3::storage::configErrorCodeToString(snapshot.storageBootstrapError);

  JsonObject queues = doc["queues"].to<JsonObject>();
  JsonObject snapshotQueue = queues["snapshot"].to<JsonObject>();
  snapshotQueue["depth"] = snapshot.queueTelemetry.snapshotQueueDepth;
  snapshotQueue["highWater"] = snapshot.queueTelemetry.snapshotQueueHighWater;
  snapshotQueue["dropCount"] = snapshot.queueTelemetry.snapshotQueueDropCount;

  JsonObject commandQueue = queues["command"].to<JsonObject>();
  commandQueue["depth"] = snapshot.queueTelemetry.commandQueueDepth;
  commandQueue["highWater"] = snapshot.queueTelemetry.commandQueueHighWater;
  commandQueue["dropCount"] = snapshot.queueTelemetry.commandQueueDropCount;
  commandQueue["appliedCount"] = snapshot.queueTelemetry.commandAppliedCount;
  commandQueue["setRunModeCount"] =
      snapshot.queueTelemetry.commandSetRunModeCount;
  commandQueue["stepCount"] = snapshot.queueTelemetry.commandStepCount;
  commandQueue["lastAppliedType"] =
      snapshot.queueTelemetry.commandLastAppliedType;
  commandQueue["lastLatencyMs"] = snapshot.queueTelemetry.commandLastLatencyMs;
  commandQueue["maxLatencyMs"] = snapshot.queueTelemetry.commandMaxLatencyMs;

  JsonObject controlQueue = queues["control"].to<JsonObject>();
  controlQueue["pendingDepth"] = snapshot.queueTelemetry.controlPendingDepth;
  controlQueue["pendingHighWater"] =
      snapshot.queueTelemetry.controlPendingHighWater;
  controlQueue["requestedCount"] = snapshot.queueTelemetry.controlRequestedCount;
  controlQueue["acceptedCount"] = snapshot.queueTelemetry.controlAcceptedCount;
  controlQueue["rejectedCount"] = snapshot.queueTelemetry.controlRejectedCount;
  controlQueue["lastRejectReason"] =
      snapshot.queueTelemetry.controlLastRejectReason;
  controlQueue["dispatchQueueFullCount"] =
      snapshot.queueTelemetry.controlDispatchQueueFullCount;

  JsonObject portalQueue = queues["portalIngress"].to<JsonObject>();
  portalQueue["requestedCount"] = snapshot.queueTelemetry.portalRequestedCount;
  portalQueue["acceptedCount"] = snapshot.queueTelemetry.portalAcceptedCount;
  portalQueue["rejectedCount"] = snapshot.queueTelemetry.portalRejectedCount;
  portalQueue["lastRejectReason"] = snapshot.queueTelemetry.portalLastRejectReason;
  portalQueue["lastRequestId"] = snapshot.queueTelemetry.portalLastRequestId;
  portalQueue["lastRequestAccepted"] =
      snapshot.queueTelemetry.portalLastRequestAccepted;
  portalQueue["queueAcceptedCount"] =
      snapshot.queueTelemetry.portalQueueAcceptedCount;
  portalQueue["queueRejectedCount"] =
      snapshot.queueTelemetry.portalQueueRejectedCount;

  JsonObject parity = queues["parity"].to<JsonObject>();
  parity["kernelLastAppliedRequestId"] =
      snapshot.queueTelemetry.kernelLastAppliedRequestId;
  parity["controlRequestedExceedsPortalAccepted"] =
      snapshot.queueTelemetry.parityControlRequestedExceedsPortalAccepted;
  parity["controlAcceptedExceedsControlRequested"] =
      snapshot.queueTelemetry.parityControlAcceptedExceedsControlRequested;
  parity["kernelAppliedExceedsControlAccepted"] =
      snapshot.queueTelemetry.parityKernelAppliedExceedsControlAccepted;

  JsonObject commandIngress = doc["commandIngress"].to<JsonObject>();
  commandIngress["pendingDepth"] = ingress_.pendingDepth;
  commandIngress["pendingHighWater"] = ingress_.pendingHighWater;
  commandIngress["requestedCount"] = ingress_.requestedCount;
  commandIngress["acceptedCount"] = ingress_.acceptedCount;
  commandIngress["rejectedCount"] = ingress_.rejectedCount;
  commandIngress["lastRejectReason"] = ingress_.lastRejectReason;
  commandIngress["lastRequestAccepted"] = ingress_.lastRequestAccepted;
  commandIngress["lastRequestId"] = ingress_.lastRequestId;
  commandIngress["queueAcceptedCount"] = ingress_.queueAcceptedCount;
  commandIngress["queueRejectedCount"] = ingress_.queueRejectedCount;

  JsonObject heapGuard = doc["heapGuard"].to<JsonObject>();
  heapGuard["diagnosticsReserveReady"] = diagnosticsReserveReady_;
  heapGuard["snapshotReserveReady"] = snapshotReserveReady_;
  heapGuard["diagnosticsSerializeFailureCount"] =
      diagnosticsSerializeFailureCount_;
  heapGuard["snapshotSerializeFailureCount"] = snapshotSerializeFailureCount_;
  heapGuard["diagnosticsCapacityRejectCount"] = diagnosticsCapacityRejectCount_;
  heapGuard["snapshotCapacityRejectCount"] = snapshotCapacityRejectCount_;
  heapGuard["diagnosticsMaxBytes"] = kDiagnosticsJsonReserve;
  heapGuard["snapshotMaxBytes"] = kSnapshotJsonReserve;

  const size_t requiredBytes = measureJson(doc);
  if ((requiredBytes + 1U) > kDiagnosticsJsonReserve) {
    diagnosticsCapacityRejectCount_ += 1;
    diagnosticsFallbackActive_ = true;
    diagnosticsRevision_ += 1;
    diagnosticsReady_ = true;
    return;
  }

  diagnosticsJson_.remove(0);
  if (serializeJson(doc, diagnosticsJson_) == 0) {
    diagnosticsSerializeFailureCount_ += 1;
    diagnosticsFallbackActive_ = true;
  } else {
    diagnosticsFallbackActive_ = false;
  }
  diagnosticsRevision_ += 1;
  diagnosticsReady_ = true;
}

void PortalService::rebuildSnapshotJson(
    const v3::runtime::RuntimeSnapshot& snapshot, const RuntimeSnapshotCard* cards,
    uint8_t cardCount) {
  JsonDocument doc;

  doc["type"] = "runtime_snapshot";
  doc["apiVersion"] = "2.0";
  doc["schemaVersion"] = 1;
  doc["snapshotSeq"] = snapshot.completedScans;
  doc["revision"] = snapshotRevision_ + 1;
  doc["tsMs"] = snapshot.nowMs;
  doc["scanPeriodMs"] = snapshot.scanPeriodMs;
  doc["lastCompleteScanMs"] = static_cast<double>(snapshot.lastScanMs) / 1000.0;
  doc["engineMode"] = toString(snapshot.mode);
  JsonObject testMode = doc["testMode"].to<JsonObject>();
  testMode["active"] = false;
  testMode["outputMaskGlobal"] = false;
  testMode["breakpointPaused"] = false;
  testMode["scanCursor"] = 0;

  JsonObject metrics = doc["metrics"].to<JsonObject>();
  metrics["scanLastUs"] = snapshot.scanLastUs;
  metrics["scanMaxUs"] = snapshot.scanMaxUs;
  metrics["scanBudgetUs"] = snapshot.scanPeriodMs * 1000U;
  metrics["scanOverrunLast"] = snapshot.scanOverrunLast;
  metrics["scanOverrunCount"] = snapshot.scanOverrunCount;
  metrics["queueDepth"] = snapshot.queueTelemetry.commandQueueDepth;
  metrics["queueHighWaterMark"] = snapshot.queueTelemetry.commandQueueHighWater;
  metrics["queueCapacity"] = snapshot.queueTelemetry.commandQueueCapacity;
  metrics["commandLatencyLastUs"] = snapshot.queueTelemetry.commandLastLatencyUs;
  metrics["commandLatencyMaxUs"] = snapshot.queueTelemetry.commandMaxLatencyUs;

  // Compatibility aliases retained during portal migration window.
  metrics["scanLastMs"] = snapshot.lastScanMs;
  metrics["scanCompleted"] = snapshot.completedScans;
  metrics["configuredCardCount"] = snapshot.configuredCardCount;
  metrics["enabledCardCount"] = snapshot.enabledCardCount;
  metrics["familyCountSum"] = snapshot.familyCountSum;
  metrics["bindingConsistent"] = snapshot.bindingConsistent;
  metrics["snapshotQueueDepth"] = snapshot.queueTelemetry.snapshotQueueDepth;
  metrics["snapshotQueueHighWater"] =
      snapshot.queueTelemetry.snapshotQueueHighWater;
  metrics["snapshotQueueDropCount"] =
      snapshot.queueTelemetry.snapshotQueueDropCount;

  JsonArray cardsJson = doc["cards"].to<JsonArray>();
  if (cards != nullptr) {
    for (uint8_t i = 0; i < cardCount; ++i) {
      const RuntimeSnapshotCard& card = cards[i];
      JsonObject item = cardsJson.add<JsonObject>();
      item["id"] = card.id;
      item["type"] = toString(card.type);
      item["familyOrder"] = card.id;
      item["index"] = card.index;
      item["commandState"] = card.commandState;
      item["actualState"] = card.actualState;
      item["edgePulse"] = card.edgePulse;
      item["state"] = toString(card.state);
      item["mode"] = toString(card.mode);
      item["liveValue"] = card.liveValue;
      item["startOnMs"] = card.startOnMs;
      item["startOffMs"] = card.startOffMs;
      item["repeatCounter"] = card.repeatCounter;
      item["turnOnConditionMet"] = card.turnOnConditionMet;
      item["turnOffConditionMet"] = card.turnOffConditionMet;
      item["breakpointEnabled"] = false;
      item["evalCounter"] = snapshot.completedScans;
      JsonObject maskForced = item["maskForced"].to<JsonObject>();
      maskForced["inputSource"] = "REAL";
      maskForced["forcedAIValue"] = 0;
      maskForced["outputMaskLocal"] = false;
      maskForced["outputMasked"] = false;
      JsonObject debug = item["debug"].to<JsonObject>();
      debug["evalCounter"] = snapshot.completedScans;
      debug["breakpointEnabled"] = false;
    }
  }

  const size_t requiredBytes = measureJson(doc);
  if ((requiredBytes + 1U) > kSnapshotJsonReserve) {
    snapshotCapacityRejectCount_ += 1;
    snapshotFallbackActive_ = true;
    snapshotRevision_ += 1;
    snapshotReady_ = true;
    return;
  }

  snapshotJson_.remove(0);
  if (serializeJson(doc, snapshotJson_) == 0) {
    snapshotSerializeFailureCount_ += 1;
    snapshotFallbackActive_ = true;
  } else {
    snapshotFallbackActive_ = false;
  }
  snapshotRevision_ += 1;
  snapshotReady_ = true;
}

bool PortalService::enqueueRequest(const PortalCommandRequest& request) {
  ingress_.requestedCount += 1;
  if (depth_ >= kPendingCapacity) {
    ingress_.rejectedCount += 1;
    ingress_.queueRejectedCount += 1;
    ingress_.lastRejectReason =
        static_cast<uint8_t>(v3::control::CommandRejectReason::QueueFull);
    ingress_.lastRequestAccepted = false;
    ingress_.lastRequestId = request.requestId;
    return false;
  }

  pending_[tail_] = request;
  tail_ = static_cast<uint16_t>((tail_ + 1) % kPendingCapacity);
  depth_ += 1;
  ingress_.pendingDepth = depth_;
  if (depth_ > ingress_.pendingHighWater) {
    ingress_.pendingHighWater = depth_;
  }
  ingress_.queueAcceptedCount += 1;
  ingress_.lastRequestAccepted = true;
  ingress_.lastRejectReason =
      static_cast<uint8_t>(v3::control::CommandRejectReason::None);
  ingress_.lastRequestId = request.requestId;
  return true;
}

}  // namespace v3::portal



