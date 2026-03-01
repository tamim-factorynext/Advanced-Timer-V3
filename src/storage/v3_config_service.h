#pragma once

#include <stddef.h>
#include <stdint.h>

#include <ArduinoJson.h>

#include "kernel/card_model.h"
#include "kernel/string_compat.h"
#include "kernel/v3_card_types.h"
#include "storage/v3_normalizer.h"

constexpr size_t kV3ConfigContextMaxCards = 255;
constexpr size_t kV3ConfigContextMaxRtcChannels = 16;

struct V3ConfigContext {
  V3CardConfig typedCards[kV3ConfigContextMaxCards];
  size_t typedCount;
  V3RtcScheduleChannel rtcChannels[kV3ConfigContextMaxRtcChannels];
  size_t rtcCount;
};

bool normalizeV3ConfigRequestTyped(
    JsonObjectConst root, const V3CardLayout& layout, const char* apiVersion,
    const char* schemaVersion, const LogicCard* baselineCards,
    size_t baselineCount, V3CardConfig* outTypedCards, size_t outTypedCount,
    V3RtcScheduleChannel* outRtc, size_t outRtcCount, String& reason,
    const char*& outErrorCode);

bool normalizeV3ConfigRequestContext(
    JsonObjectConst root, const V3CardLayout& layout, const char* apiVersion,
    const char* schemaVersion, const LogicCard* baselineCards,
    size_t baselineCount, size_t rtcChannelCount, V3ConfigContext& outContext,
    String& reason, const char*& outErrorCode);

bool buildLegacyCardsFromTypedWithBaseline(const V3CardConfig* typedCards,
                                           size_t typedCount,
                                           const LogicCard* baselineCards,
                                           size_t baselineCount,
                                           LogicCard* outCards, String& reason);

void applyRtcScheduleChannelsFromConfig(const V3RtcScheduleChannel* source,
                                        size_t sourceCount,
                                        V3RtcScheduleChannel* target,
                                        size_t targetCount);
