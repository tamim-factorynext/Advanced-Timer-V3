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
