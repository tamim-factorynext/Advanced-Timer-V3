#include "portal/transport_command_stub.h"

#include <ArduinoJson.h>
#include <cstring>

namespace v3::portal {

namespace {

// Canonical transport status mapping:
// - 200: submit accepted
// - 429: submit rejected by queue/flow-control
// - 400: malformed payload shape/json
// - 422: semantically invalid command content

const char* sourceToString(TransportCommandSource source) {
  switch (source) {
    case TransportCommandSource::Http:
      return "http";
    case TransportCommandSource::WebSocket:
      return "websocket";
    default:
      return "unknown";
  }
}

const char* rejectReasonToString(v3::control::CommandRejectReason reason) {
  switch (reason) {
    case v3::control::CommandRejectReason::None:
      return "none";
    case v3::control::CommandRejectReason::QueueFull:
      return "queue_full";
    case v3::control::CommandRejectReason::InvalidRunMode:
      return "invalid_run_mode";
    case v3::control::CommandRejectReason::InvalidInputMode:
      return "invalid_input_mode";
    case v3::control::CommandRejectReason::StepRequiresRunStep:
      return "step_requires_run_step";
    default:
      return "unknown";
  }
}

const char* errorCodeToMessage(const char* errorCode) {
  if (errorCode == nullptr) return "Unknown error.";
  if (strcmp(errorCode, "none") == 0) return "No error.";
  if (strcmp(errorCode, "missing_payload") == 0) return "Missing payload body.";
  if (strcmp(errorCode, "invalid_json") == 0) return "Payload is not valid JSON.";
  if (strcmp(errorCode, "missing_command") == 0) return "Command field is missing.";
  if (strcmp(errorCode, "invalid_run_mode") == 0)
    return "Run mode value is invalid.";
  if (strcmp(errorCode, "invalid_card_id") == 0)
    return "Card ID is missing or out of range.";
  if (strcmp(errorCode, "invalid_input_mode") == 0)
    return "Input force mode must be REAL, FORCED_HIGH, or FORCED_LOW.";
  if (strcmp(errorCode, "invalid_input_value") == 0)
    return "Input force value is missing or invalid.";
  if (strcmp(errorCode, "unsupported_command") == 0)
    return "Command is not supported.";
  if (strcmp(errorCode, "queue_full") == 0)
    return "Command queue is currently full.";
  if (strcmp(errorCode, "step_requires_run_step") == 0)
    return "Step command requires RUN_STEP mode.";
  return "Command request failed.";
}

bool parseRunMode(const char* rawMode, runMode& outMode) {
  if (rawMode == nullptr) return false;
  if (strcmp(rawMode, "RUN_NORMAL") == 0) {
    outMode = RUN_NORMAL;
    return true;
  }
  if (strcmp(rawMode, "RUN_STEP") == 0) {
    outMode = RUN_STEP;
    return true;
  }
  if (strcmp(rawMode, "RUN_BREAKPOINT") == 0) {
    outMode = RUN_BREAKPOINT;
    return true;
  }
  return false;
}

bool parseInputMode(const char* rawMode, inputSourceMode& outMode) {
  if (rawMode == nullptr) return false;
  if (strcmp(rawMode, "REAL") == 0) {
    outMode = InputSource_Real;
    return true;
  }
  if (strcmp(rawMode, "FORCED_HIGH") == 0) {
    outMode = InputSource_ForcedHigh;
    return true;
  }
  if (strcmp(rawMode, "FORCED_LOW") == 0) {
    outMode = InputSource_ForcedLow;
    return true;
  }
  return false;
}

TransportCommandResponse buildError(uint16_t statusCode, const char* source,
                                    const char* errorCode) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["accepted"] = false;
  doc["requestId"] = 0;
  doc["reason"] = "error";
  doc["source"] = source;
  doc["errorCode"] = errorCode;
  doc["message"] = errorCodeToMessage(errorCode);

  String body;
  serializeJson(doc, body);
  return {statusCode, body};
}

TransportCommandResponse buildSubmitResult(uint16_t statusCode, const char* source,
                                           const PortalCommandSubmitResult& r) {
  JsonDocument doc;
  doc["ok"] = (statusCode < 400);
  doc["accepted"] = r.accepted;
  doc["requestId"] = r.requestId;
  doc["reason"] = rejectReasonToString(r.reason);
  doc["source"] = source;
  const char* errorCode = rejectReasonToString(r.reason);
  doc["errorCode"] = errorCode;
  doc["message"] = errorCodeToMessage(errorCode);

  String body;
  serializeJson(doc, body);
  return {statusCode, body};
}

}  // namespace

TransportCommandResponse handleTransportCommandStub(
    PortalService& portal, const char* payloadJson, TransportCommandSource source,
    uint32_t nowUs) {
  const char* sourceLabel = sourceToString(source);
  if (payloadJson == nullptr) {
    return buildError(400, sourceLabel, "missing_payload");
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, payloadJson);
  if (error) {
    return buildError(400, sourceLabel, "invalid_json");
  }

  const char* command = doc["command"].as<const char*>();
  if (command == nullptr) {
    return buildError(400, sourceLabel, "missing_command");
  }

  PortalCommandSubmitResult submit = {};
  if (strcmp(command, "setRunMode") == 0) {
    runMode mode = RUN_NORMAL;
    if (!parseRunMode(doc["mode"].as<const char*>(), mode)) {
      return buildError(422, sourceLabel, "invalid_run_mode");
    }
    submit = portal.submitSetRunMode(mode, nowUs);
  } else if (strcmp(command, "stepOnce") == 0) {
    submit = portal.submitStepOnce(nowUs);
  } else if (strcmp(command, "setDiForce") == 0) {
    if (!doc["cardId"].is<uint8_t>()) {
      return buildError(422, sourceLabel, "invalid_card_id");
    }
    inputSourceMode inputMode = InputSource_Real;
    if (!parseInputMode(doc["mode"].as<const char*>(), inputMode)) {
      return buildError(422, sourceLabel, "invalid_input_mode");
    }
    submit = portal.submitSetInputForce(doc["cardId"].as<uint8_t>(), inputMode, 0U,
                                        nowUs);
  } else if (strcmp(command, "setAiForce") == 0) {
    if (!doc["cardId"].is<uint8_t>()) {
      return buildError(422, sourceLabel, "invalid_card_id");
    }
    if (!doc["value"].is<uint32_t>()) {
      return buildError(422, sourceLabel, "invalid_input_value");
    }
    submit = portal.submitSetInputForce(doc["cardId"].as<uint8_t>(),
                                        InputSource_ForcedValue,
                                        doc["value"].as<uint32_t>(), nowUs);
  } else if (strcmp(command, "clearAiForce") == 0) {
    if (!doc["cardId"].is<uint8_t>()) {
      return buildError(422, sourceLabel, "invalid_card_id");
    }
    submit = portal.submitSetInputForce(doc["cardId"].as<uint8_t>(),
                                        InputSource_Real, 0U, nowUs);
  } else {
    return buildError(422, sourceLabel, "unsupported_command");
  }

  const uint16_t statusCode = submit.accepted ? 200 : 429;
  return buildSubmitResult(statusCode, sourceLabel, submit);
}

}  // namespace v3::portal
