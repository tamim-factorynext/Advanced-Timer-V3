#include "storage/v3_config_service.h"

#include <cstring>
#include <string>

#include "kernel/v3_card_bridge.h"
#include "kernel/v3_config_sanitize.h"
#include "kernel/v3_typed_config_rules.h"

bool normalizeV3ConfigRequestTyped(
    JsonObjectConst root, const V3CardLayout& layout, const char* apiVersion,
    const char* schemaVersion, const LogicCard* baselineCards,
    size_t baselineCount, V3CardConfig* outTypedCards, size_t outTypedCount,
    V3RtcScheduleChannel* outRtc, size_t outRtcCount, String& reason,
    const char*& outErrorCode) {
  JsonDocument normalizedDoc;
  JsonArrayConst normalizedCards;
  if (!normalizeConfigRequestWithLayout(
          root, layout, apiVersion, schemaVersion, baselineCards, baselineCount,
          normalizedDoc, normalizedCards, reason, outErrorCode, outRtc,
          outRtcCount, outTypedCards, outTypedCount)) {
    return false;
  }

  std::string typedReason;
  if (!validateTypedCardConfigs(outTypedCards, static_cast<uint8_t>(layout.totalCards),
                                layout.doStart, layout.aiStart, layout.sioStart,
                                layout.mathStart, layout.rtcStart, typedReason)) {
    reason = typedReason.c_str();
    outErrorCode = "VALIDATION_FAILED";
    return false;
  }
  return true;
}

bool normalizeV3ConfigRequestContext(
    JsonObjectConst root, const V3CardLayout& layout, const char* apiVersion,
    const char* schemaVersion, const LogicCard* baselineCards,
    size_t baselineCount, size_t rtcChannelCount, V3ConfigContext& outContext,
    String& reason, const char*& outErrorCode) {
  if (layout.totalCards > kV3ConfigContextMaxCards) {
    reason = "layout totalCards exceeds context capacity";
    outErrorCode = "INTERNAL_ERROR";
    return false;
  }
  if (rtcChannelCount > kV3ConfigContextMaxRtcChannels) {
    reason = "rtc channel count exceeds context capacity";
    outErrorCode = "INTERNAL_ERROR";
    return false;
  }

  outContext.typedCount = layout.totalCards;
  outContext.rtcCount = rtcChannelCount;
  return normalizeV3ConfigRequestTyped(
      root, layout, apiVersion, schemaVersion, baselineCards, baselineCount,
      outContext.typedCards, outContext.typedCount, outContext.rtcChannels,
      outContext.rtcCount, reason, outErrorCode);
}

bool buildLegacyCardsFromTypedWithBaseline(const V3CardConfig* typedCards,
                                           size_t typedCount,
                                           const LogicCard* baselineCards,
                                           size_t baselineCount,
                                           LogicCard* outCards,
                                           String& reason) {
  if (typedCards == nullptr || baselineCards == nullptr || outCards == nullptr) {
    reason = "typed/baseline/out buffer missing";
    return false;
  }
  if (typedCount == 0 || typedCount != baselineCount) {
    reason = "typed/baseline count mismatch";
    return false;
  }

  std::memcpy(outCards, baselineCards, sizeof(LogicCard) * typedCount);
  for (size_t i = 0; i < typedCount; ++i) {
    if (!v3CardConfigToLegacy(typedCards[i], outCards[i])) {
      reason = "failed to map typed cards into runtime cards";
      return false;
    }
  }
  sanitizeConfigCardsRuntimeFields(outCards, static_cast<uint8_t>(typedCount));
  reason = "";
  return true;
}

void applyRtcScheduleChannelsFromConfig(const V3RtcScheduleChannel* source,
                                        size_t sourceCount,
                                        V3RtcScheduleChannel* target,
                                        size_t targetCount) {
  if (source == nullptr || target == nullptr) return;
  const size_t count = (sourceCount < targetCount) ? sourceCount : targetCount;
  for (size_t i = 0; i < count; ++i) {
    target[i].enabled = source[i].enabled;
    target[i].year = source[i].year;
    target[i].month = source[i].month;
    target[i].day = source[i].day;
    target[i].weekday = source[i].weekday;
    target[i].hour = source[i].hour;
    target[i].minute = source[i].minute;
    target[i].rtcCardId = source[i].rtcCardId;
  }
}
