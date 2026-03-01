#pragma once

#include <stdint.h>

#include <string>

#include <ArduinoJson.h>

bool validateV3PayloadConditionSources(JsonArrayConst cards, uint8_t totalCards,
                                       uint8_t doStart, uint8_t aiStart,
                                       uint8_t sioStart, uint8_t mathStart,
                                       uint8_t rtcStart, std::string& reason);

