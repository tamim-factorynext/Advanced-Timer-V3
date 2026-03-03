/*
File: src/portal/transport_command_stub.h
Purpose: Declares the transport command stub module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/portal/transport_command_stub.cpp
- src/portal/transport_runtime.cpp
Flow Hook:
- Portal request handling and runtime snapshot transport.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
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
