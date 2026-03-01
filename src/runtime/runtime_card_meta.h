#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct RuntimeCardMeta {
  uint8_t id;
  logicCardType type;
  uint8_t index;
  cardMode mode;
};

RuntimeCardMeta makeRuntimeCardMeta(const LogicCard& card);
void refreshRuntimeCardMetaFromCards(const LogicCard* cards, uint8_t count,
                                     RuntimeCardMeta* out);
