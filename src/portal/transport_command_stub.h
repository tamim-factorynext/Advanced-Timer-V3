#pragma once

#include <Arduino.h>

#include "portal/portal_service.h"

namespace v3::portal {

enum class TransportCommandSource : uint8_t {
  Http,
  WebSocket,
};

struct TransportCommandResponse {
  uint16_t statusCode;
  String body;
};

TransportCommandResponse handleTransportCommandStub(
    PortalService& portal, const char* payloadJson,
    TransportCommandSource source, uint32_t nowUs);

}  // namespace v3::portal
