#pragma once

#include <stdint.h>

enum runMode { RUN_NORMAL, RUN_STEP, RUN_BREAKPOINT };

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
  uint32_t enqueuedUs;
  runMode mode;
  inputSourceMode inputMode;
};
