#include "portal/portal_service.h"

namespace v3::portal {

void PortalService::begin() {
  lastTickMs_ = 0;
  observedScanCount_ = 0;
}

void PortalService::tick(uint32_t nowMs,
                         const v3::runtime::RuntimeSnapshot& snapshot) {
  lastTickMs_ = nowMs;
  observedScanCount_ = snapshot.completedScans;
}

}  // namespace v3::portal
