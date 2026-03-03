/*
File: src/kernel/v3_card_bridge.h
Purpose: Declares the v3 card bridge module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/v3_card_bridge.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include "kernel/card_model.h"
#include "kernel/v3_card_types.h"

/**
 * @brief Converts legacy `LogicCard` into typed V3 card config.
 * @details Bridges legacy persisted/runtime data into V3 typed family contract.
 * @par Used By
 * - src/kernel/v3_card_bridge.cpp
 */
bool legacyToV3CardConfig(const LogicCard& legacy, const int16_t rtcYear,
                          const int8_t rtcMonth, const int8_t rtcDay,
                          const int8_t rtcWeekday, const int8_t rtcHour,
                          const int8_t rtcMinute, V3CardConfig& out);

/**
 * @brief Converts typed V3 card config into legacy `LogicCard`.
 * @details Supports compatibility paths that still consume legacy card model.
 * @par Used By
 * - src/kernel/v3_card_bridge.cpp
 */
bool v3CardConfigToLegacy(const V3CardConfig& v3, LogicCard& out);

