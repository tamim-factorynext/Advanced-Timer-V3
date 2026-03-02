#pragma once

#include <stdint.h>

#include "storage/v3_config_contract.h"

namespace v3::platform {

enum class WiFiState : uint8_t {
  Offline,
  ConnectingMaster,
  ConnectingUser,
  StaConnected,
};

enum class WiFiConnectPhase : uint8_t {
  None,
  Master,
  User,
};

struct WiFiStatus {
  WiFiState state;
  WiFiConnectPhase phase;
  bool staConnected;
  bool online;
  uint32_t lastAttemptMs;
  uint32_t phaseStartedMs;
  uint32_t retryCount;
  uint32_t offlineSinceMs;
  char staIp[16];
};

class WiFiRuntime {
 public:
  void begin(const v3::storage::WiFiConfig& config);
  void tick(uint32_t nowMs);
  const WiFiStatus& status() const;

 private:
  void startMasterAttempt(uint32_t nowMs);
  void startUserAttempt(uint32_t nowMs);
  void enterOffline(uint32_t nowMs);
  void refreshStaIp();

  v3::storage::WiFiConfig config_ = {};
  WiFiStatus status_ = {};
};

}  // namespace v3::platform
