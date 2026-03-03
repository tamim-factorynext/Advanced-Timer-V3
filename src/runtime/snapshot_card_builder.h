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

RuntimeSnapshotCard buildRuntimeSnapshotCard(const RuntimeCardMeta& meta,
                                             const V3RuntimeStoreView& store);
void buildRuntimeSnapshotCards(const RuntimeCardMeta* meta, uint8_t count,
                               const V3RuntimeStoreView& store,
                               RuntimeSnapshotCard* outCards);
