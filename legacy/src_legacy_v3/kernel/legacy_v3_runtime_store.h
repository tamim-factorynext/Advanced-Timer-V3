/*
File: src/kernel/legacy_v3_runtime_store.h
Purpose: Declares the v3 runtime store module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/legacy_v3_runtime_signals.h
- src/runtime/snapshot_card_builder.h
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/legacy_v3_card_types.h"
#include "kernel/legacy_v3_ai_runtime.h"
#include "kernel/legacy_v3_di_runtime.h"
#include "kernel/legacy_v3_do_runtime.h"
#include "kernel/legacy_v3_math_runtime.h"
#include "kernel/legacy_v3_rtc_runtime.h"
#include "kernel/legacy_v3_sio_runtime.h"

/**
 * @brief Mutable view over per-family V3 runtime state arrays.
 * @details Provides one consolidated store pointer set used by signal/snapshot helpers.
 * @par Used By
 * - src/kernel/legacy_v3_runtime_store.cpp
 * - src/kernel/legacy_v3_runtime_signals.cpp
 * - src/runtime/snapshot_card_builder.cpp
 */
struct V3RuntimeStoreView {
  V3DiRuntimeState* di;
  uint8_t diCount;
  V3DoRuntimeState* dOut;
  uint8_t dOutCount;
  V3AiRuntimeState* ai;
  uint8_t aiCount;
  V3SioRuntimeState* sio;
  uint8_t sioCount;
  V3MathRuntimeState* math;
  uint8_t mathCount;
  V3RtcRuntimeState* rtc;
  uint8_t rtcCount;
};

/**
 * @brief Synchronizes runtime store state from typed card config + legacy card array.
 * @details Initializes runtime state arrays before scan execution begins.
 * @par Used By
 * - src/kernel/legacy_v3_runtime_store.cpp
 */
void syncRuntimeStoreFromTypedCards(const LogicCard* cards,
                                    const V3CardConfig* typedCards,
                                    uint8_t cardCount,
                                    const V3RuntimeStoreView& store);
/**
 * @brief Mirrors one runtime store card state back to legacy card representation.
 * @details Maintains compatibility surfaces that still consume `LogicCard` state.
 * @par Used By
 * - src/runtime/snapshot_card_builder.cpp
 */
void mirrorRuntimeStoreCardToLegacyByTyped(LogicCard& card,
                                           const V3CardConfig& typedCard,
                                           const V3RuntimeStoreView& store);

/** @brief Returns DI runtime-state pointer by index or null when out-of-range. */
V3DiRuntimeState* runtimeDiStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store);
/** @brief Returns DO runtime-state pointer by index or null when out-of-range. */
V3DoRuntimeState* runtimeDoStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store);
/** @brief Returns AI runtime-state pointer by index or null when out-of-range. */
V3AiRuntimeState* runtimeAiStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store);
/** @brief Returns SIO runtime-state pointer by index or null when out-of-range. */
V3SioRuntimeState* runtimeSioStateAt(uint8_t index,
                                     const V3RuntimeStoreView& store);
/** @brief Returns MATH runtime-state pointer by index or null when out-of-range. */
V3MathRuntimeState* runtimeMathStateAt(uint8_t index,
                                       const V3RuntimeStoreView& store);
/** @brief Returns RTC runtime-state pointer by index or null when out-of-range. */
V3RtcRuntimeState* runtimeRtcStateAt(uint8_t index,
                                     const V3RuntimeStoreView& store);


