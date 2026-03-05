#pragma once

#include "core/contracts/core_service.hpp"

namespace at {
namespace runtime {

class ServiceManager {
 public:
  static constexpr uint8_t kMaxServices = 8;

  bool registerService(core::CoreService *service);
  void initAll();
  void tickAll(uint32_t nowMs);

 private:
  core::CoreService *services_[kMaxServices] = {};
  uint8_t serviceCount_ = 0;
};

}  // namespace runtime
}  // namespace at

