#include "core/runtime/service_manager.hpp"

namespace at {
namespace runtime {

bool ServiceManager::registerService(core::CoreService *service) {
  if (service == nullptr || serviceCount_ >= kMaxServices) {
    return false;
  }
  services_[serviceCount_++] = service;
  return true;
}

void ServiceManager::initAll() {
  for (uint8_t i = 0; i < serviceCount_; ++i) {
    services_[i]->init();
  }
}

void ServiceManager::tickAll(uint32_t nowMs) {
  for (uint8_t i = 0; i < serviceCount_; ++i) {
    services_[i]->tick(nowMs);
  }
}

}  // namespace runtime
}  // namespace at

