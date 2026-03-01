#pragma once

#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/v3_card_types.h"

struct RuntimeCardMeta {
  uint8_t id;
  logicCardType type;
  uint8_t index;
  cardMode mode;
};

void refreshRuntimeCardMetaFromTypedCards(
    const V3CardConfig* cards, uint8_t count, uint8_t doStart, uint8_t aiStart,
    uint8_t sioStart, uint8_t mathStart, uint8_t rtcStart, RuntimeCardMeta* out);
