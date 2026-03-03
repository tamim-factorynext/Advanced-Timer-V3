/*
File: src/runtime/runtime_card_meta.h
Purpose: Declares the runtime card meta module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/v3_runtime_signals.h
- src/runtime/runtime_card_meta.cpp
- src/runtime/snapshot_card_builder.h
Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/v3_card_types.h"

/** @brief Lightweight per-card metadata used for runtime snapshot projection. */
struct RuntimeCardMeta {
  uint8_t id;
  logicCardType type;
  uint8_t index;
  cardMode mode;
};

/**
 * @brief Rebuilds runtime-card metadata array from typed card config.
 * @details Produces stable identity/mode/index mapping used by snapshot/signal builders.
 * @par Used By
 * - src/runtime/runtime_card_meta.cpp
 * - src/runtime/snapshot_card_builder.cpp
 */
void refreshRuntimeCardMetaFromTypedCards(
    const V3CardConfig* cards, uint8_t count, uint8_t doStart, uint8_t aiStart,
    uint8_t sioStart, uint8_t mathStart, uint8_t rtcStart, RuntimeCardMeta* out);
