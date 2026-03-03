/*
File: src/storage/config_lifecycle.h
Purpose: Declares the config lifecycle module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- firmware build target
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/string_compat.h"

bool writeJsonToPath(const char* path, JsonDocument& doc);
bool readJsonFromPath(const char* path, JsonDocument& doc);
bool saveCardsToPath(const char* path, const LogicCard* sourceCards);
bool loadCardsFromPath(const char* path, LogicCard* outCards);
bool copyFileIfExists(const char* srcPath, const char* dstPath);
void formatVersion(char* out, size_t outSize, uint32_t version);
bool pauseKernelForConfigApply(uint32_t timeoutMs);
void resumeKernelAfterConfigApply();
void rotateHistoryVersions();
bool applyCardsAsActiveConfig(const LogicCard* newCards);
bool extractConfigCardsFromRequest(JsonObjectConst root, JsonArrayConst& outCards,
                                   String& reason);
void writeConfigErrorResponse(int statusCode, const char* code,
                              const String& message);
bool loadPortalSettingsFromLittleFS();
bool savePortalSettingsToLittleFS();
