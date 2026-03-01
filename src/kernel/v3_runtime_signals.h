#pragma once

#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/v3_runtime_store.h"
#include "runtime/runtime_card_meta.h"

struct V3RuntimeSignal {
  logicCardType type;
  cardState state;
  bool logicalState;
  bool physicalState;
  bool triggerFlag;
  uint32_t currentValue;
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
