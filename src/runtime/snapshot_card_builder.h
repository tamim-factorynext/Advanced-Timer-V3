/*
File: src/runtime/snapshot_card_builder.h
Purpose: Declares the snapshot card builder module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/runtime/runtime_card_meta.h
Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/v3_runtime_store.h"
#include "runtime/runtime_card_meta.h"
#include "runtime/runtime_snapshot_card.h"

/**
 * @brief Builds one runtime snapshot card row from metadata and runtime store.
 * @details Converts family-specific runtime state into transport-friendly snapshot schema.
 * @par Used By
 * - src/main.cpp
 * - src/runtime/snapshot_card_builder.cpp
 */
RuntimeSnapshotCard buildRuntimeSnapshotCard(const RuntimeCardMeta& meta,
                                             const V3RuntimeStoreView& store);
/**
 * @brief Builds full runtime snapshot card array.
 * @details Iterates metadata rows and calls per-card builder helper.
 * @par Used By
 * - src/runtime/snapshot_card_builder.cpp
 */
void buildRuntimeSnapshotCards(const RuntimeCardMeta* meta, uint8_t count,
                               const V3RuntimeStoreView& store,
                               RuntimeSnapshotCard* outCards);
