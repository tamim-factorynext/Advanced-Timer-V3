/*
File: src/control/command_dto.h
Purpose: Declares the command dto module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src\control\command_dto.h
- src\control\control_service.h
- src\kernel\enum_codec.h
- src\kernel\kernel_service.h
- (+ more call sites)

Flow Hook:
- Control command orchestration and system-level actions.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

enum engineMode { RUN_NORMAL, RUN_STEP, RUN_BREAKPOINT };

enum inputSourceMode {
  InputSource_Real,
  InputSource_ForcedHigh,
  InputSource_ForcedLow,
  InputSource_ForcedValue
};

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

