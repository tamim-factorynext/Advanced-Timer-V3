/*
File: src/storage/storage_service.h
Purpose: Declares the storage service module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/main.cpp
- src/runtime/runtime_service.h
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include "storage/v3_config_validator.h"

namespace v3::storage {

/**
 * @brief Identifies where active config was bootstrapped from.
 * @details Distinguishes default-contract bootstrap from filesystem-backed bootstrap.
 * @par Used By
 * - src/storage/storage_service.cpp
 * - src/runtime/runtime_service.cpp
 */
enum class BootstrapSource : uint8_t { DefaultConfig, FileConfig };

/**
 * @brief Summarizes startup configuration bootstrap outcome.
 * @details Combines source, validation/decode error state, and active-config availability.
 * @par Used By
 * - src/main.cpp
 * - src/runtime/runtime_service.cpp
 */
struct BootstrapDiagnostics {
  BootstrapSource source;
  ConfigValidationError error;
  bool hasActiveConfig;
};

/**
 * @brief Owns configuration bootstrap, validation gate, and active config access.
 * @details Loads candidate config (file/default), validates, and exposes stable runtime config view.
 * @par Used By
 * - src/main.cpp
 * - src/runtime/runtime_service.h
 */
class StorageService {
 public:
  /**
   * @brief Bootstraps active config and captures startup diagnostics.
   * @details Performs filesystem load attempt and validator gate before exposing config.
   * @par Used By
   * - src/main.cpp
   */
  void begin();
  /**
   * @brief Indicates whether a validated active config is available.
   * @details False means startup should fail-fast or apply explicit fallback policy.
   * @return `true` when `activeConfig()` is valid to consume.
   * @par Used By
   * - src/main.cpp
   */
  bool hasActiveConfig() const;
  /**
   * @brief Returns validated active system config.
   * @details Caller must check `hasActiveConfig()` before use.
   * @return Immutable validated config view.
   * @par Used By
   * - src/main.cpp
   * - src/kernel/kernel_service.cpp
   */
  const ValidatedConfig& activeConfig() const;
  /**
   * @brief Returns last bootstrap/validation error.
   * @details Useful for startup fail diagnostics and operator telemetry.
   * @return Validation/decode error descriptor.
   * @par Used By
   * - src/main.cpp
   */
  const ConfigValidationError& lastError() const;
  /**
   * @brief Produces bootstrap diagnostics aggregate.
   * @details Used by runtime snapshot projection and observability paths.
   * @return Bootstrap diagnostics value.
   * @par Used By
   * - src/main.cpp
   * - src/runtime/runtime_service.cpp
   */
  BootstrapDiagnostics diagnostics() const;

 private:
  BootstrapSource source_ = BootstrapSource::DefaultConfig;
  ValidatedConfig activeConfig_ = {};
  ConfigValidationError lastError_ = {ConfigErrorCode::None, 0};
  bool activeConfigPresent_ = false;
};

}  // namespace v3::storage
