/*
File: src/runtime/legacy_snapshot_json.h
Purpose: Declares the snapshot json module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- firmware build target
Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <ArduinoJson.h>
#include <stdint.h>

struct SharedRuntimeSnapshot;

/**
 * @brief Copies shared runtime snapshot from producer-owned store.
 * @details Provides a stable copy point before JSON serialization routines read fields.
 * @par Used By
 * - src/runtime/legacy_snapshot_json.cpp (or equivalent translation unit)
 */
void legacyCopySharedRuntimeSnapshot(SharedRuntimeSnapshot& outSnapshot);
/**
 * @brief Appends one card snapshot entry into JSON array.
 * @details Serializes card fields from shared snapshot view for a specific card id.
 * @par Used By
 * - src/runtime/legacy_snapshot_json.cpp (or equivalent translation unit)
 */
void legacyAppendRuntimeSnapshotCard(JsonArray& cards,
                                     const SharedRuntimeSnapshot& snapshot,
                                     uint8_t cardId);
/**
 * @brief Serializes full runtime snapshot JSON document.
 * @details Writes top-level metrics plus per-card entries into provided JsonDocument.
 * @par Used By
 * - src/runtime/legacy_snapshot_json.cpp (or equivalent translation unit)
 */
void legacySerializeRuntimeSnapshot(JsonDocument& doc, uint32_t nowMs);

