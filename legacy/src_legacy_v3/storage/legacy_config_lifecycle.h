/*
File: src/storage/legacy_config_lifecycle.h
Purpose: Declares the config lifecycle module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- firmware build target
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/string_compat.h"

bool legacyWriteJsonToPath(const char* path, JsonDocument& doc);
bool legacyReadJsonFromPath(const char* path, JsonDocument& doc);
bool legacySaveCardsToPath(const char* path, const LogicCard* sourceCards);
bool legacyLoadCardsFromPath(const char* path, LogicCard* outCards);
bool legacyCopyFileIfExists(const char* srcPath, const char* dstPath);
void legacyFormatVersion(char* out, size_t outSize, uint32_t version);
bool legacyPauseKernelForConfigApply(uint32_t timeoutMs);
void legacyResumeKernelAfterConfigApply();
void legacyRotateHistoryVersions();
bool legacyApplyCardsAsActiveConfig(const LogicCard* newCards);
bool legacyExtractConfigCardsFromRequest(JsonObjectConst root,
                                         JsonArrayConst& outCards,
                                         String& reason);
void legacyWriteConfigErrorResponse(int statusCode, const char* code,
                                    const String& message);
bool legacyLoadPortalSettingsFromLittleFS();
bool legacySavePortalSettingsToLittleFS();

