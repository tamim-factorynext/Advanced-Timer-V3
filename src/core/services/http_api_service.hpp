#pragma once

#include <WebServer.h>

#include "core/contracts/core_service.hpp"
#include "core/runtime/status_snapshot.hpp"

namespace at {
namespace services {

class HttpApiService : public core::CoreService {
 public:
  explicit HttpApiService(runtime::StatusSnapshotModel &statusModel);

  const char *name() const override;
  void init() override;
  void tick(uint32_t nowMs) override;

 private:
  void handleStatus();
  void handleNotFound();
  void ensureServerStarted();

  runtime::StatusSnapshotModel &statusModel_;
  WebServer server_;
  bool serverStarted_ = false;
};

}  // namespace services
}  // namespace at

