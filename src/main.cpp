#include <Arduino.h>

#include "control/control_service.h"
#include "kernel/kernel_service.h"
#include "platform/platform_service.h"
#include "portal/portal_service.h"
#include "runtime/runtime_service.h"
#include "storage/storage_service.h"

namespace {

v3::platform::PlatformService gPlatform;
v3::storage::StorageService gStorage;
v3::kernel::KernelService gKernel;
v3::runtime::RuntimeService gRuntime;
v3::control::ControlService gControl;
v3::portal::PortalService gPortal;

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Advanced Timer V3 Core bootstrap");

  gPlatform.begin();
  gStorage.begin();

  if (!gStorage.hasActiveConfig()) {
    const v3::storage::ConfigValidationError err = gStorage.lastError();
    Serial.print("V3 config bootstrap failed: ");
    Serial.println(v3::storage::configErrorCodeToString(err.code));
    while (true) {
      delay(1000);
    }
  }

  gKernel.begin(gStorage.activeConfig());

  gRuntime.begin();
  gControl.begin();
  gPortal.begin();
}

void loop() {
  const uint32_t nowMs = gPlatform.nowMs();

  gControl.tick(nowMs);
  gKernel.tick(nowMs);
  gRuntime.tick(nowMs, gKernel.metrics());
  gPortal.tick(nowMs, gRuntime.snapshot());

  delay(1);
}
