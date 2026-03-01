#pragma once

#include <stdint.h>

#include <ArduinoJson.h>

#include "kernel/card_model.h"
#include "kernel/string_compat.h"
#include "kernel/v3_card_types.h"

bool parseV3CardToTyped(JsonObjectConst v3Card, const logicCardType* sourceTypeById,
                        uint8_t totalCards, uint8_t doStart, uint8_t aiStart,
                        uint8_t sioStart, uint8_t mathStart, uint8_t rtcStart,
                        V3CardConfig& out, String& reason);
