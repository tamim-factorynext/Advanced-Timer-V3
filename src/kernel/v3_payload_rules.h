/*
File: src/kernel/v3_payload_rules.h
Purpose: Declares the v3 payload rules module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/storage/v3_normalizer.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include <string>

#include <ArduinoJson.h>

/**
 * @brief Validates condition source references inside V3 payload card definitions.
 * @details Enforces source-card compatibility and card-range constraints before typed decode.
 * @param cards Payload `cards` array.
 * @param totalCards Expected total card count.
 * @param doStart Family boundary index for DO cards.
 * @param aiStart Family boundary index for AI cards.
 * @param sioStart Family boundary index for SIO cards.
 * @param mathStart Family boundary index for MATH cards.
 * @param rtcStart Family boundary index for RTC cards.
 * @param reason Receives failure reason text when validation fails.
 * @return `true` when payload condition source references are valid.
 * @par Used By
 * - src/storage/v3_normalizer.cpp
 */
bool validateV3PayloadConditionSources(JsonArrayConst cards, uint8_t totalCards,
                                       uint8_t doStart, uint8_t aiStart,
                                       uint8_t sioStart, uint8_t mathStart,
                                       uint8_t rtcStart, std::string& reason);
