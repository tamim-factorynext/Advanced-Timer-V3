/*
File: src/kernel/v3_runtime_signals.h
Purpose: Declares the v3 runtime signals module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/kernel/v3_runtime_signals.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/v3_runtime_store.h"
#include "runtime/runtime_card_meta.h"

struct V3RuntimeSignal {
  logicCardType type;
  cardState state;
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
};

V3RuntimeSignal makeRuntimeSignal(const RuntimeCardMeta& meta,
                                  const V3RuntimeStoreView& store);
void refreshRuntimeSignalsFromRuntime(const RuntimeCardMeta* cardsMeta,
                                      const V3RuntimeStoreView& store,
                                      V3RuntimeSignal* out, uint8_t count);
void refreshRuntimeSignalAt(const RuntimeCardMeta* cardsMeta,
                            const V3RuntimeStoreView& store,
                            V3RuntimeSignal* out, uint8_t count,
                            uint8_t cardId);

