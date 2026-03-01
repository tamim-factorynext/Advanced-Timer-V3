#pragma once

#include <stdint.h>

#include <ArduinoJson.h>

#include "kernel/string_compat.h"

bool validateLegacyConfigCardsArray(JsonArrayConst array, uint8_t totalCards,
                                    uint8_t doStart, uint8_t aiStart,
                                    uint8_t sioStart, uint8_t mathStart,
                                    uint8_t rtcStart, String& reason);
