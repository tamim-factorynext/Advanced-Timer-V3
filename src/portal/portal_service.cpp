#include "portal/portal_service.h"

#include <ArduinoJson.h>

namespace v3::portal {

void PortalService::begin() {
  lastTickMs_ = 0;
  observedScanCount_ = 0;
  diagnosticsRevision_ = 0;
  diagnosticsReady_ = false;
  diagnosticsJson_[0] = '\0';
  head_ = 0;
  tail_ = 0;
  depth_ = 0;
  nextRequestId_ = 0;
  ingress_ = {};
}

void PortalService::tick(uint32_t nowMs,
                         const v3::runtime::RuntimeSnapshot& snapshot) {
  lastTickMs_ = nowMs;
  observedScanCount_ = snapshot.completedScans;
  rebuildDiagnosticsJson(snapshot);
}

bool PortalService::enqueueSetRunModeRequest(runMode mode, uint32_t requestId,
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

PortalCommandSubmitResult PortalService::submitSetRunMode(runMode mode,
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
  state.json = diagnosticsJson_;
  return state;
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

  serializeJson(doc, diagnosticsJson_, sizeof(diagnosticsJson_));
  diagnosticsRevision_ += 1;
  diagnosticsReady_ = true;
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
