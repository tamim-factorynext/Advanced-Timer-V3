#include "kernel/legacy_card_profile.h"

#include "kernel/enum_codec.h"
#include "kernel/v3_config_sanitize.h"

void profileSerializeCardToJson(const LogicCard& card, JsonObject& json) {
  json["id"] = card.id;
  json["type"] = toString(card.type);
  json["index"] = card.index;
  json["hwPin"] = card.hwPin;
  json["invert"] = card.invert;

  json["setting1"] = card.setting1;
  json["setting2"] = card.setting2;
  if (card.type == AnalogInput) {
    json["setting3"] = static_cast<double>(card.setting3) / 1000.0;
  } else {
    json["setting3"] = card.setting3;
  }

  json["mode"] = toString(card.mode);
  if (card.type == AnalogInput || card.type == MathCard) {
    json["startOnMs"] = card.startOnMs;
    json["startOffMs"] = card.startOffMs;
  }

  json["setA_ID"] = card.setA_ID;
  json["setA_Operator"] = toString(card.setA_Operator);
  json["setA_Threshold"] = card.setA_Threshold;
  json["setB_ID"] = card.setB_ID;
  json["setB_Operator"] = toString(card.setB_Operator);
  json["setB_Threshold"] = card.setB_Threshold;
  json["setCombine"] = toString(card.setCombine);

  json["resetA_ID"] = card.resetA_ID;
  json["resetA_Operator"] = toString(card.resetA_Operator);
  json["resetA_Threshold"] = card.resetA_Threshold;
  json["resetB_ID"] = card.resetB_ID;
  json["resetB_Operator"] = toString(card.resetB_Operator);
  json["resetB_Threshold"] = card.resetB_Threshold;
  json["resetCombine"] = toString(card.resetCombine);
}

void profileDeserializeCardFromJson(JsonVariantConst jsonVariant, LogicCard& card) {
  if (!jsonVariant.is<JsonObjectConst>()) return;
  JsonObjectConst json = jsonVariant.as<JsonObjectConst>();
  LogicCard before = card;

  card.id = json["id"] | card.id;
  const char* rawType = json["type"].as<const char*>();
  logicCardType parsedType = before.type;
  bool typeOk = tryParseLogicCardType(rawType, parsedType);
  card.type = typeOk ? parsedType : before.type;
  card.index = json["index"] | card.index;
  card.hwPin = json["hwPin"] | card.hwPin;
  card.invert = json["invert"] | card.invert;

  card.setting1 = json["setting1"] | card.setting1;
  card.setting2 = json["setting2"] | card.setting2;
  if (card.type == AnalogInput) {
    const double currentAlpha = static_cast<double>(card.setting3) / 1000.0;
    const double parsed = json["setting3"] | currentAlpha;
    if (parsed >= 0.0 && parsed <= 1.0) {
      const double scaled = parsed * 1000.0;
      card.setting3 = static_cast<uint32_t>(scaled + 0.5);
    } else {
      // Backward compatibility: accept legacy milliunit payloads.
      int32_t legacy = static_cast<int32_t>(parsed);
      if (legacy < 0) legacy = 0;
      if (legacy > 1000) legacy = 1000;
      card.setting3 = static_cast<uint32_t>(legacy);
    }
  } else {
    card.setting3 = json["setting3"] | card.setting3;
  }

  if (card.type == AnalogInput || card.type == MathCard) {
    card.startOnMs = json["startOnMs"] | card.startOnMs;
    card.startOffMs = json["startOffMs"] | card.startOffMs;
  }

  const char* rawMode = json["mode"].as<const char*>();
  cardMode parsedMode = before.mode;
  bool modeOk = tryParseCardMode(rawMode, parsedMode);
  card.mode = modeOk ? parsedMode : before.mode;

  card.setA_ID = json["setA_ID"] | card.setA_ID;
  const char* rawSetA = json["setA_Operator"].as<const char*>();
  logicOperator parsedSetA = before.setA_Operator;
  bool setAOk = tryParseLogicOperator(rawSetA, parsedSetA);
  card.setA_Operator = setAOk ? parsedSetA : before.setA_Operator;
  card.setA_Threshold = json["setA_Threshold"] | card.setA_Threshold;
  card.setB_ID = json["setB_ID"] | card.setB_ID;
  const char* rawSetB = json["setB_Operator"].as<const char*>();
  logicOperator parsedSetB = before.setB_Operator;
  bool setBOk = tryParseLogicOperator(rawSetB, parsedSetB);
  card.setB_Operator = setBOk ? parsedSetB : before.setB_Operator;
  card.setB_Threshold = json["setB_Threshold"] | card.setB_Threshold;
  const char* rawSetCombine = json["setCombine"].as<const char*>();
  combineMode parsedSetCombine = before.setCombine;
  bool setCombineOk = tryParseCombineMode(rawSetCombine, parsedSetCombine);
  card.setCombine = setCombineOk ? parsedSetCombine : before.setCombine;

  card.resetA_ID = json["resetA_ID"] | card.resetA_ID;
  const char* rawResetA = json["resetA_Operator"].as<const char*>();
  logicOperator parsedResetA = before.resetA_Operator;
  bool resetAOk = tryParseLogicOperator(rawResetA, parsedResetA);
  card.resetA_Operator = resetAOk ? parsedResetA : before.resetA_Operator;
  card.resetA_Threshold = json["resetA_Threshold"] | card.resetA_Threshold;
  card.resetB_ID = json["resetB_ID"] | card.resetB_ID;
  const char* rawResetB = json["resetB_Operator"].as<const char*>();
  logicOperator parsedResetB = before.resetB_Operator;
  bool resetBOk = tryParseLogicOperator(rawResetB, parsedResetB);
  card.resetB_Operator = resetBOk ? parsedResetB : before.resetB_Operator;
  card.resetB_Threshold = json["resetB_Threshold"] | card.resetB_Threshold;
  const char* rawResetCombine = json["resetCombine"].as<const char*>();
  combineMode parsedResetCombine = before.resetCombine;
  bool resetCombineOk =
      tryParseCombineMode(rawResetCombine, parsedResetCombine);
  card.resetCombine = resetCombineOk ? parsedResetCombine : before.resetCombine;
}

void profileInitializeCardSafeDefaults(LogicCard& card, uint8_t globalId,
                                       const LegacyCardProfileLayout& layout) {
  card.id = globalId;
  card.invert = false;
  card.setting1 = 0;
  card.setting2 = 0;
  card.setting3 = 0;
  card.logicalState = false;
  card.physicalState = false;
  card.triggerFlag = false;
  card.currentValue = 0;
  card.startOnMs = 0;
  card.startOffMs = 0;
  card.repeatCounter = 0;

  card.setA_ID = globalId;
  card.setB_ID = globalId;
  card.resetA_ID = globalId;
  card.resetB_ID = globalId;
  card.setA_Operator = Op_AlwaysFalse;
  card.setB_Operator = Op_AlwaysFalse;
  card.resetA_Operator = Op_AlwaysFalse;
  card.resetB_Operator = Op_AlwaysFalse;
  card.setA_Threshold = 0;
  card.setB_Threshold = 0;
  card.resetA_Threshold = 0;
  card.resetB_Threshold = 0;
  card.setCombine = Combine_None;
  card.resetCombine = Combine_None;

  if (globalId < layout.doStart) {
    card.type = DigitalInput;
    card.index = globalId;
    card.hwPin = layout.diPins[card.index];
    card.setting1 = 50;
    card.setting2 = 0;
    card.setting3 = 0;
    card.mode = Mode_DI_Rising;
    card.state = State_DI_Idle;
    return;
  }

  if (globalId < layout.aiStart) {
    card.type = DigitalOutput;
    card.index = static_cast<uint8_t>(globalId - layout.doStart);
    card.hwPin = layout.doPins[card.index];
    card.setting1 = 1000;
    card.setting2 = 1000;
    card.setting3 = 1;
    card.mode = Mode_DO_Normal;
    card.state = State_DO_Idle;
    return;
  }

  if (globalId < layout.sioStart) {
    card.type = AnalogInput;
    card.index = static_cast<uint8_t>(globalId - layout.aiStart);
    card.hwPin = layout.aiPins[card.index];
    card.setting1 = 0;
    card.setting2 = 4095;
    card.setting3 = 250;
    card.startOnMs = 0;
    card.startOffMs = 10000;
    card.mode = Mode_AI_Continuous;
    card.state = State_AI_Streaming;
    return;
  }

  if (globalId < layout.mathStart) {
    card.type = SoftIO;
    card.index = static_cast<uint8_t>(globalId - layout.sioStart);
    card.hwPin = layout.sioPins[card.index];
    card.setting1 = 1000;
    card.setting2 = 1000;
    card.setting3 = 1;
    card.mode = Mode_DO_Normal;
    card.state = State_DO_Idle;
    return;
  }

  if (globalId < layout.rtcStart) {
    card.type = MathCard;
    card.index = static_cast<uint8_t>(globalId - layout.mathStart);
    card.hwPin = 255;
    card.setting1 = 0;
    card.setting2 = 0;
    card.setting3 = 0;
    card.startOnMs = 0;
    card.startOffMs = 0;
    card.mode = Mode_None;
    card.state = State_None;
    return;
  }

  card.type = RtcCard;
  card.index = static_cast<uint8_t>(globalId - layout.rtcStart);
  card.hwPin = 255;
  card.setting1 = 60000;
  card.setting2 = 0;
  card.setting3 = 0;
  card.mode = Mode_None;
  card.state = State_None;
}

void profileInitializeCardArraySafeDefaults(LogicCard* cards,
                                            const LegacyCardProfileLayout& layout) {
  for (uint8_t i = 0; i < layout.totalCards; ++i) {
    profileInitializeCardSafeDefaults(cards[i], i, layout);
  }
}

bool profileDeserializeCardsFromArray(JsonArrayConst array, LogicCard* outCards,
                                      const LegacyCardProfileLayout& layout) {
  if (array.size() != layout.totalCards) return false;
  profileInitializeCardArraySafeDefaults(outCards, layout);
  for (uint8_t i = 0; i < layout.totalCards; ++i) {
    JsonVariantConst item = array[i];
    if (!item.is<JsonObjectConst>()) return false;
    profileDeserializeCardFromJson(item, outCards[i]);
  }
  sanitizeConfigCardsRuntimeFields(outCards, layout.totalCards);
  return true;
}
