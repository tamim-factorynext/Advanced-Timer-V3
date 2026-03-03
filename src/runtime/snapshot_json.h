/*
File: src/runtime/snapshot_json.h
Purpose: Declares the snapshot json module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- firmware build target
Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <ArduinoJson.h>
#include <stdint.h>

struct SharedRuntimeSnapshot;

void copySharedRuntimeSnapshot(SharedRuntimeSnapshot& outSnapshot);
void appendRuntimeSnapshotCard(JsonArray& cards,
                               const SharedRuntimeSnapshot& snapshot,
                               uint8_t cardId);
void serializeRuntimeSnapshot(JsonDocument& doc, uint32_t nowMs);
