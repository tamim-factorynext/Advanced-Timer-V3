#pragma once

#include <stdint.h>

#include <string>

#include "kernel/v3_card_types.h"

bool validateTypedCardConfigs(const V3CardConfig* cards, uint8_t count,
                              uint8_t doStart, uint8_t aiStart,
                              uint8_t sioStart, uint8_t mathStart,
                              uint8_t rtcStart, std::string& reason);
