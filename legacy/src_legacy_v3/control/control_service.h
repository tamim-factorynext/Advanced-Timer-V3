/*
File: src/control/control_service.h
Purpose: Declares the control service module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/control/control_service.cpp
- src/main.cpp
- src/portal/portal_service.h
Flow Hook:
- Control command orchestration and system-level actions.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "control/command_dto.h"

namespace v3::control {

/**
 * @brief Describes why a control request was rejected.
 * @details Rejection reasons are surfaced to portal/runtime diagnostics for operator visibility.
 * @par Used By
 * - src/control/control_service.cpp
 * - src/portal/portal_service.h
 */
enum class CommandRejectReason : uint8_t {
  None,
  QueueFull,
  InvalidRunMode,
  InvalidInputMode,
  StepRequiresRunStep,
};

/**
 * @brief Runtime counters for control ingress and queue behavior.
 * @details Provides observability for request acceptance/rejection and queue backpressure.
 * @par Used By
 * - src/control/control_service.cpp
 * - src/main.cpp
 * - src/runtime/runtime_service.cpp
 */
struct ControlDiagnostics {
  uint16_t pendingDepth;
  uint16_t pendingHighWater;
  uint32_t requestedCount;
  uint32_t acceptedCount;
  uint32_t rejectedCount;
  CommandRejectReason lastRejectReason;
};

/**
 * @brief Owns validation and buffering of portal-issued control commands.
 * @details Converts portal intents into queue-ready `KernelCommand` messages.
 * @par Used By
 * - src/main.cpp
 * - src/portal/portal_service.h
 */
class ControlService {
 public:
  /**
   * @brief Resets internal state and diagnostics counters.
   * @details Must be called once during startup before enqueue/dequeue operations.
   * @par Used By
   * - src/main.cpp
   */
  void begin();
  /**
   * @brief Advances control service housekeeping state.
   * @details Updates internal time marker used for service cadence/diagnostics.
   * @param nowMs Current platform timestamp in milliseconds.
   * @par Used By
   * - src/main.cpp
   */
  void tick(uint32_t nowMs);
  /**
   * @brief Requests transition of kernel engine run mode.
   * @details Validates run mode and queues command if pending capacity permits.
   * @param mode Desired run mode.
   * @param nowUs Enqueue timestamp used for latency accounting.
   * @param requestId Optional external trace id.
   * @return `true` if accepted into pending queue.
   * @par Used By
   * - src/main.cpp
   */
  bool requestSetRunMode(engineMode mode, uint32_t nowUs, uint32_t requestId = 0);
  /**
   * @brief Requests one deterministic step execution.
   * @details Accepted only when current mode policy allows step operation.
   * @param nowUs Enqueue timestamp used for latency accounting.
   * @param requestId Optional external trace id.
   * @return `true` if accepted into pending queue.
   * @par Used By
   * - src/main.cpp
   */
  bool requestStepOnce(uint32_t nowUs, uint32_t requestId = 0);
  /**
   * @brief Requests DI/AI input source forcing for a target card.
   * @details Converts typed forcing intent into kernel command DTO.
   * @param cardId Target card id.
   * @param inputMode Selected forcing mode.
   * @param nowUs Enqueue timestamp used for latency accounting.
   * @param requestId Optional external trace id.
   * @param forcedValue Numeric forced value for value-mode forcing.
   * @return `true` if accepted into pending queue.
   * @par Used By
   * - src/main.cpp
   */
  bool requestSetInputForce(uint8_t cardId, inputSourceMode inputMode,
                            uint32_t nowUs, uint32_t requestId = 0,
                            uint32_t forcedValue = 0);
  /**
   * @brief Pops one queued command for dispatch toward kernel queue boundary.
   * @details Non-blocking dequeue; caller handles downstream queue admission.
   * @param out Receives dequeued command when available.
   * @return `true` when a command was dequeued.
   * @par Used By
   * - src/main.cpp
   */
  bool dequeueCommand(KernelCommand& out);
  /**
   * @brief Returns current control diagnostics snapshot.
   * @details Exposes queue depth/high-water and acceptance/rejection counters.
   * @return Immutable diagnostics view.
   * @par Used By
   * - src/main.cpp
   * - src/runtime/runtime_service.cpp
   */
  const ControlDiagnostics& diagnostics() const;

 private:
  bool enqueueCommand(const KernelCommand& command);
  static bool isValidRunMode(engineMode mode);

  static constexpr uint16_t kPendingCapacity = 16;
  KernelCommand pending_[kPendingCapacity] = {};
  uint16_t head_ = 0;
  uint16_t tail_ = 0;
  uint16_t depth_ = 0;
  uint32_t lastTickMs_ = 0;
  engineMode desiredMode_ = RUN_NORMAL;
  ControlDiagnostics diagnostics_ = {};
};

}  // namespace v3::control


