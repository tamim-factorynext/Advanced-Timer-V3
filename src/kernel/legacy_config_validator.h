/*
File: src/kernel/legacy_config_validator.h
Purpose: Declares the legacy config validator module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/legacy_config_validator.cpp
- src/kernel/legacy_config_validator.h

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include <ArduinoJson.h>

#include "kernel/string_compat.h"

bool validateLegacyConfigCardsArray(JsonArrayConst array, uint8_t totalCards,
                                    uint8_t doStart, uint8_t aiStart,
                                    uint8_t sioStart, uint8_t mathStart,
                                    uint8_t rtcStart, String& reason);
