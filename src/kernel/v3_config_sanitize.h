/*
File: src/kernel/v3_config_sanitize.h
Purpose: Declares the v3 config sanitize module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/legacy_card_profile.cpp
- src/kernel/v3_config_sanitize.cpp
- src/kernel/v3_config_sanitize.h

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

void sanitizeConfigCardRuntimeFields(LogicCard& card);
void sanitizeConfigCardsRuntimeFields(LogicCard* cards, uint8_t count);
