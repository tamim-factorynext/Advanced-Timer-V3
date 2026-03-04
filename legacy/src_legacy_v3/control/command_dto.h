/*
File: src/control/command_dto.h
Purpose: Declares the command dto module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/control/control_service.h
- src/kernel/enum_codec.h
- src/kernel/kernel_service.h
- (+ more call sites)
Flow Hook:
- Control command orchestration and system-level actions.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

/**
 * @brief Defines deterministic kernel run mode.
 * @details Controls how scan ticks are consumed by the kernel engine.
 * @par Used By
 * - src/control/control_service.h
 * - src/kernel/kernel_service.h
 * - src/main.cpp
 */
enum engineMode { RUN_NORMAL, RUN_STEP, RUN_BREAKPOINT };

/**
 * @brief Selects DI/AI input source policy for command-driven forcing.
 * @details Determines whether input values come from real hardware or forced values.
 * @par Used By
 * - src/control/control_service.h
 * - src/main.cpp
 */
enum inputSourceMode {
  InputSource_Real,
  InputSource_ForcedHigh,
  InputSource_ForcedLow,
  InputSource_ForcedValue
};

/**
 * @brief Enumerates command intents that can be applied by kernel command path.
 * @details Represents the cross-core command contract between control and kernel tasks.
 * @par Used By
 * - src/control/control_service.h
 * - src/main.cpp
 */
enum kernelCommandType {
  KernelCmd_SetRunMode,
  KernelCmd_StepOnce,
  KernelCmd_SetBreakpoint,
  KernelCmd_SetTestMode,
  KernelCmd_SetInputForce,
  KernelCmd_SetOutputMask,
  KernelCmd_SetOutputMaskGlobal,
  KernelCmd_SetRtcCardState
};

/**
 * @brief Encapsulates one kernel command message for queue handoff.
 * @details Carries typed command intent plus metadata for latency and request tracking.
 * @par Field Semantics
 * - `type`: command intent selector.
 * - `cardId`: target card for card-scoped commands.
 * - `value`: numeric payload for value-carrying commands.
 * - `requestId`: portal/control trace identifier.
 * - `enqueuedUs`: enqueue timestamp used for command latency metrics.
 * - `mode`: desired engine mode for run-mode commands.
 * - `inputMode`: selected input forcing mode for input-force commands.
 * @par Used By
 * - src/control/control_service.cpp
 * - src/main.cpp
 */
struct KernelCommand {
  kernelCommandType type;
  uint8_t cardId;
  bool flag;
  uint32_t value;
  uint32_t requestId;
  uint32_t enqueuedUs;
  engineMode mode;
  inputSourceMode inputMode;
};


