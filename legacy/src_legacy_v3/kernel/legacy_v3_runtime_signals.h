/*
File: src/kernel/legacy_v3_runtime_signals.h
Purpose: Declares the v3 runtime signals module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/kernel/legacy_v3_runtime_signals.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/legacy_v3_runtime_store.h"
#include "runtime/runtime_card_meta.h"

/** @brief Normalized runtime signal row used by condition evaluators. */
struct V3RuntimeSignal {
  logicCardType type;
  cardState state;
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
};

/**
 * @brief Builds one runtime signal row from card metadata and runtime store view.
 * @details Bridges runtime-state representation into condition-evaluator input contract.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 * - src/kernel/legacy_v3_runtime_signals.cpp
 */
V3RuntimeSignal makeRuntimeSignal(const RuntimeCardMeta& meta,
                                  const V3RuntimeStoreView& store);
/**
 * @brief Refreshes runtime signal array from metadata/store snapshots.
 * @details Bulk helper used once per scan before evaluating condition blocks.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
void refreshRuntimeSignalsFromRuntime(const RuntimeCardMeta* cardsMeta,
                                      const V3RuntimeStoreView& store,
                                      V3RuntimeSignal* out, uint8_t count);
/**
 * @brief Refreshes one runtime signal entry by card id.
 * @details Targeted helper for incremental signal updates.
 * @par Used By
 * - src/kernel/legacy_v3_runtime_signals.cpp
 */
void refreshRuntimeSignalAt(const RuntimeCardMeta* cardsMeta,
                            const V3RuntimeStoreView& store,
                            V3RuntimeSignal* out, uint8_t count,
                            uint8_t cardId);



