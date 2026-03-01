#pragma once

#include <stdint.h>

#include <ArduinoJson.h>

#include "kernel/card_model.h"

struct LegacyCardProfileLayout {
  uint8_t totalCards;
  uint8_t doStart;
  uint8_t aiStart;
  uint8_t sioStart;
  uint8_t mathStart;
  uint8_t rtcStart;
  const uint8_t* diPins;
  const uint8_t* doPins;
  const uint8_t* aiPins;
  const uint8_t* sioPins;
};

void profileSerializeCardToJson(const LogicCard& card, JsonObject& json);
void profileDeserializeCardFromJson(JsonVariantConst jsonVariant, LogicCard& card);

void profileInitializeCardSafeDefaults(LogicCard& card, uint8_t globalId,
                                       const LegacyCardProfileLayout& layout);
void profileInitializeCardArraySafeDefaults(LogicCard* cards,
                                            const LegacyCardProfileLayout& layout);

bool profileDeserializeCardsFromArray(JsonArrayConst array, LogicCard* outCards,
                                      const LegacyCardProfileLayout& layout);
