#pragma once

#include <stddef.h>
#include <stdint.h>

#include <ArduinoJson.h>

#include "kernel/card_model.h"
#include "kernel/string_compat.h"
#include "kernel/v3_card_types.h"

struct V3CardLayout {
  uint8_t totalCards;
  uint8_t doStart;
  uint8_t aiStart;
  uint8_t sioStart;
  uint8_t mathStart;
  uint8_t rtcStart;
};

struct V3RtcScheduleChannel {
  bool enabled;
  int16_t year;
  int8_t month;
  int8_t day;
  int8_t weekday;
  int8_t hour;
  int8_t minute;
  uint8_t rtcCardId;
};

bool normalizeConfigRequestWithLayout(
    JsonObjectConst root, const V3CardLayout& layout, const char* apiVersion,
    const char* schemaVersion, const LogicCard* baselineCards,
    size_t baselineCount, JsonDocument& normalizedDoc, JsonArrayConst& outCards,
    String& reason, const char*& outErrorCode, V3RtcScheduleChannel* rtcOut,
    size_t rtcOutCount, V3CardConfig* typedOut, size_t typedOutCount);
