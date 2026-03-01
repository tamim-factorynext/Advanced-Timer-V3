#pragma once

#include "kernel/card_model.h"
#include "kernel/v3_card_types.h"

bool legacyToV3CardConfig(const LogicCard& legacy, const int16_t rtcYear,
                          const int8_t rtcMonth, const int8_t rtcDay,
                          const int8_t rtcWeekday, const int8_t rtcHour,
                          const int8_t rtcMinute, V3CardConfig& out);

bool v3CardConfigToLegacy(const V3CardConfig& v3, LogicCard& out);

