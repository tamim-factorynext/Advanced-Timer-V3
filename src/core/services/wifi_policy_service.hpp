#pragma once

#include <WiFi.h>
#include <stdint.h>

#include "core/contracts/core_service.hpp"

namespace at {
namespace services {

struct WifiPolicyConfig {
  const char *backupSsid;
  const char *backupPass;
  uint32_t backupTimeoutMs;
  const char *userSsid;
  const char *userPass;
  uint32_t userTimeoutMs;
  uint32_t radioCooldownMs;
  uint32_t offlineRetryDelayMs;

  WifiPolicyConfig(const char *backupSsidIn,
                   const char *backupPassIn,
                   uint32_t backupTimeoutMsIn,
                   const char *userSsidIn,
                   const char *userPassIn,
                   uint32_t userTimeoutMsIn,
                   uint32_t radioCooldownMsIn,
                   uint32_t offlineRetryDelayMsIn)
      : backupSsid(backupSsidIn),
        backupPass(backupPassIn),
        backupTimeoutMs(backupTimeoutMsIn),
        userSsid(userSsidIn),
        userPass(userPassIn),
        userTimeoutMs(userTimeoutMsIn),
        radioCooldownMs(radioCooldownMsIn),
        offlineRetryDelayMs(offlineRetryDelayMsIn) {}
};

class WifiPolicyService : public core::CoreService {
 public:
  explicit WifiPolicyService(const WifiPolicyConfig &config);

  const char *name() const override;
  void init() override;
  void tick(uint32_t nowMs) override;

 private:
  enum class State : uint8_t {
    IdleStartBackup,
    WaitingBackup,
    StartUser,
    WaitingUser,
    Connected,
    RadioCooldown,
    OfflineBackoff
  };

  void startWifiAttempt(const char *ssid, const char *pass, uint32_t nowMs);
  void scheduleCooldownThen(State nextState, uint32_t nowMs);

  WifiPolicyConfig config_;
  State state_ = State::IdleStartBackup;
  State nextStateAfterCooldown_ = State::IdleStartBackup;
  uint32_t stateStartedMs_ = 0;
};

}  // namespace services
}  // namespace at
