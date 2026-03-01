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
