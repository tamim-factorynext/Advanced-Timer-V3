/*
File: src/kernel/v3_typed_card_parser.h
Purpose: Declares the v3 typed card parser module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/v3_typed_card_parser.cpp
- src/storage/v3_normalizer.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include <ArduinoJson.h>

#include "kernel/card_model.h"
#include "kernel/string_compat.h"
#include "kernel/v3_card_types.h"

/**
 * @brief Parses one V3 card JSON object into typed card config.
 * @details Applies family-aware field parsing and operator/source compatibility checks.
 * @param v3Card One card JSON payload object.
 * @param sourceTypeById Source family lookup table indexed by card id.
 * @param totalCards Total configured cards in payload.
 * @param doStart Family boundary index for DO cards.
 * @param aiStart Family boundary index for AI cards.
 * @param sioStart Family boundary index for SIO cards.
 * @param mathStart Family boundary index for MATH cards.
 * @param rtcStart Family boundary index for RTC cards.
 * @param out Parsed typed card result.
 * @param reason Receives parse/validation reason on failure.
 * @return `true` on successful parse.
 * @par Used By
 * - src/storage/v3_normalizer.cpp
 */
bool parseV3CardToTyped(JsonObjectConst v3Card, const logicCardType* sourceTypeById,
                        uint8_t totalCards, uint8_t doStart, uint8_t aiStart,
                        uint8_t sioStart, uint8_t mathStart, uint8_t rtcStart,
                        V3CardConfig& out, String& reason);
