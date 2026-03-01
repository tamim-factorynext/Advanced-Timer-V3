#include "runtime/runtime_card_meta.h"

RuntimeCardMeta makeRuntimeCardMeta(const LogicCard& card) {
  RuntimeCardMeta meta = {};
  meta.id = card.id;
  meta.type = card.type;
  meta.index = card.index;
  meta.mode = card.mode;
  return meta;
}

void refreshRuntimeCardMetaFromCards(const LogicCard* cards, uint8_t count,
                                     RuntimeCardMeta* out) {
  for (uint8_t i = 0; i < count; ++i) {
    out[i] = makeRuntimeCardMeta(cards[i]);
  }
}
