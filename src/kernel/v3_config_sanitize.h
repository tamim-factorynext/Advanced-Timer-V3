#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

void sanitizeConfigCardRuntimeFields(LogicCard& card);
void sanitizeConfigCardsRuntimeFields(LogicCard* cards, uint8_t count);
