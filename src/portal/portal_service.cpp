#include "portal/portal_service.h"

#include <ArduinoJson.h>

namespace v3::portal {

void PortalService::begin() {
  lastTickMs_ = 0;
  observedScanCount_ = 0;
  diagnosticsRevision_ = 0;
  diagnosticsReady_ = false;
  diagnosticsJson_[0] = '\0';
}

void PortalService::tick(uint32_t nowMs,
                         const v3::runtime::RuntimeSnapshot& snapshot) {
  lastTickMs_ = nowMs;
  observedScanCount_ = snapshot.completedScans;
  rebuildDiagnosticsJson(snapshot);
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

  JsonObject bootstrap = doc["bootstrap"].to<JsonObject>();
  bootstrap["usedFileConfig"] = snapshot.bootstrapUsedFileConfig;
  bootstrap["hasActiveConfig"] = snapshot.storageHasActiveConfig;
  bootstrap["errorCode"] =
      v3::storage::configErrorCodeToString(snapshot.storageBootstrapError);

  serializeJson(doc, diagnosticsJson_, sizeof(diagnosticsJson_));
  diagnosticsRevision_ += 1;
  diagnosticsReady_ = true;
}

}  // namespace v3::portal
