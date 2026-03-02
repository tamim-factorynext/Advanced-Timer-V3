#pragma once

#include <stdint.h>

#include "runtime/runtime_service.h"

namespace v3::portal {

struct PortalDiagnosticsState {
  bool ready;
  uint32_t revision;
  const char* json;
};

class PortalService {
 public:
  void begin();
  void tick(uint32_t nowMs, const v3::runtime::RuntimeSnapshot& snapshot);
  PortalDiagnosticsState diagnosticsState() const;

 private:
  void rebuildDiagnosticsJson(const v3::runtime::RuntimeSnapshot& snapshot);

  uint32_t lastTickMs_ = 0;
  uint32_t observedScanCount_ = 0;
  uint32_t diagnosticsRevision_ = 0;
  bool diagnosticsReady_ = false;
  char diagnosticsJson_[512] = {};
};

}  // namespace v3::portal
