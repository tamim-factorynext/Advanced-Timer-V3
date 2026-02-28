#pragma once

#include <ArduinoJson.h>
#include <stdint.h>

struct SharedRuntimeSnapshot;

void copySharedRuntimeSnapshot(SharedRuntimeSnapshot& outSnapshot);
void appendRuntimeSnapshotCard(JsonArray& cards,
                               const SharedRuntimeSnapshot& snapshot,
                               uint8_t cardId);
void serializeRuntimeSnapshot(JsonDocument& doc, uint32_t nowMs);
