#include "runtime/runtime_card_meta.h"

#include <stddef.h>

namespace {
logicCardType typeFromFamily(V3CardFamily family) {
  switch (family) {
    case V3CardFamily::DI:
      return DigitalInput;
    case V3CardFamily::DO:
      return DigitalOutput;
    case V3CardFamily::AI:
      return AnalogInput;
    case V3CardFamily::SIO:
      return SoftIO;
    case V3CardFamily::MATH:
      return MathCard;
    case V3CardFamily::RTC:
      return RtcCard;
    default:
      return DigitalInput;
  }
}

cardMode modeFromTyped(const V3CardConfig& card) {
  switch (card.family) {
    case V3CardFamily::DI:
      return card.di.edgeMode;
    case V3CardFamily::DO:
      return card.dout.mode;
    case V3CardFamily::AI:
      return Mode_AI_Continuous;
    case V3CardFamily::SIO:
      return card.sio.mode;
    case V3CardFamily::MATH:
    case V3CardFamily::RTC:
      return Mode_None;
    default:
      return Mode_None;
  }
}

uint8_t indexFromTyped(const V3CardConfig& card, uint8_t doStart, uint8_t aiStart,
                       uint8_t sioStart, uint8_t mathStart, uint8_t rtcStart) {
  switch (card.family) {
    case V3CardFamily::DI:
      return card.di.channel;
    case V3CardFamily::DO:
      return card.dout.channel;
    case V3CardFamily::AI:
      return card.ai.channel;
    case V3CardFamily::SIO:
      return (card.cardId >= sioStart) ? static_cast<uint8_t>(card.cardId - sioStart)
                                       : 0;
    case V3CardFamily::MATH:
      return (card.cardId >= mathStart)
                 ? static_cast<uint8_t>(card.cardId - mathStart)
                 : 0;
    case V3CardFamily::RTC:
      return (card.cardId >= rtcStart) ? static_cast<uint8_t>(card.cardId - rtcStart)
                                       : 0;
    default:
      return (card.cardId < doStart)
                 ? card.cardId
                 : ((card.cardId < aiStart) ? static_cast<uint8_t>(card.cardId - doStart)
                                            : 0);
  }
}
}  // namespace

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

void refreshRuntimeCardMetaFromTypedCards(
    const V3CardConfig* cards, uint8_t count, uint8_t doStart, uint8_t aiStart,
    uint8_t sioStart, uint8_t mathStart, uint8_t rtcStart, RuntimeCardMeta* out) {
  if (cards == nullptr || out == nullptr) return;
  for (uint8_t i = 0; i < count; ++i) {
    const V3CardConfig& in = cards[i];
    RuntimeCardMeta meta = {};
    meta.id = in.cardId;
    meta.type = typeFromFamily(in.family);
    meta.index =
        indexFromTyped(in, doStart, aiStart, sioStart, mathStart, rtcStart);
    meta.mode = modeFromTyped(in);
    out[i] = meta;
  }
}
