/*
File: src/kernel/v3_typed_config_rules.h
Purpose: Declares the v3 typed config rules module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/v3_typed_config_rules.cpp
- src/storage/v3_config_service.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include <string>

#include "kernel/v3_card_types.h"

/**
 * @brief Validates typed card array against fixed family layout and per-family rules.
 * @details Enforces card ordering, family-slot consistency, mode validity, and condition compatibility.
 * @param cards Typed card array.
 * @param count Active typed card count.
 * @param doStart Family boundary index for DO cards.
 * @param aiStart Family boundary index for AI cards.
 * @param sioStart Family boundary index for SIO cards.
 * @param mathStart Family boundary index for MATH cards.
 * @param rtcStart Family boundary index for RTC cards.
 * @param reason Receives failure reason text when validation fails.
 * @return `true` when typed config passes all rule checks.
 * @par Used By
 * - src/storage/v3_config_service.cpp
 */
bool validateTypedCardConfigs(const V3CardConfig* cards, uint8_t count,
                              uint8_t doStart, uint8_t aiStart,
                              uint8_t sioStart, uint8_t mathStart,
                              uint8_t rtcStart, std::string& reason);
