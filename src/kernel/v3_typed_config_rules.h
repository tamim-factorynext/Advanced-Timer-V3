/*
File: src/kernel/v3_typed_config_rules.h
Purpose: Declares the v3 typed config rules module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/v3_typed_config_rules.cpp
- src/kernel/v3_typed_config_rules.h
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

bool validateTypedCardConfigs(const V3CardConfig* cards, uint8_t count,
                              uint8_t doStart, uint8_t aiStart,
                              uint8_t sioStart, uint8_t mathStart,
                              uint8_t rtcStart, std::string& reason);
