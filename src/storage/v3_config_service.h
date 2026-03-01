#pragma once

#include <stddef.h>
#include <stdint.h>

#include <Arduino.h>
#include <ArduinoJson.h>

#include "kernel/card_model.h"
#include "kernel/v3_card_types.h"
#include "storage/v3_normalizer.h"

bool normalizeV3ConfigRequestTyped(
    JsonObjectConst root, const V3CardLayout& layout, const char* apiVersion,
    const char* schemaVersion, const LogicCard* baselineCards,
    size_t baselineCount, V3CardConfig* outTypedCards, size_t outTypedCount,
    V3RtcScheduleChannel* outRtc, size_t outRtcCount, String& reason,
    const char*& outErrorCode);

bool buildLegacyCardsFromTypedWithBaseline(const V3CardConfig* typedCards,
                                           size_t typedCount,
                                           const LogicCard* baselineCards,
                                           size_t baselineCount,
                                           LogicCard* outCards, String& reason);
