/*
File: src/portal/transport_runtime.cpp
Purpose: Implements the transport runtime module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/main.cpp
Flow Hook:
- Portal request handling and runtime snapshot transport.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "portal/transport_runtime.h"

#include <WebServer.h>
#include <WebSocketsServer.h>

#include "portal/transport_command_stub.h"

namespace v3::portal {

namespace {

WebServer gHttpServer(80);
WebSocketsServer gWsServer(81);
PortalService* gPortal = nullptr;
bool gTransportInitialized = false;

/**
 * @brief Handles HTTP command submit endpoint.
 * @details Delegates payload parsing/execution to transport command stub and
 * writes JSON response.
 * @par Used By
 * `POST /api/v3/command`.
 */
void handleHttpCommandSubmit() {
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }

  const String payload = gHttpServer.arg("plain");
  const TransportCommandResponse response = handleTransportCommandStub(
      *gPortal, payload.c_str(), TransportCommandSource::Http, micros());
  gHttpServer.send(response.statusCode, "application/json", response.body);
}

/**
 * @brief Handles HTTP runtime snapshot endpoint.
 * @par Used By
 * `GET /api/v3/snapshot`.
 */
void handleHttpSnapshotGet() {
  if (gPortal == nullptr) {
    gHttpServer.send(500, "application/json",
                     "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
    return;
  }
  const PortalSnapshotState state = gPortal->snapshotState();
  if (!state.ready || state.json == nullptr || state.json[0] == '\0') {
    gHttpServer.send(503, "application/json",
                     "{\"ok\":false,\"reason\":\"snapshot_not_ready\"}");
    return;
  }
  gHttpServer.send(200, "application/json", state.json);
}

/**
 * @brief Handles inbound WebSocket command messages.
 * @details Accepts text frames only and responds directly to originating
 * client with command result JSON.
 * @param clientNum WebSocket client id.
 * @param type Event type.
 * @param payload Raw payload bytes.
 * @param length Payload length.
 */
void onWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload,
                      size_t length) {
  if (gPortal == nullptr) return;
  if (type != WStype_TEXT) return;

  String text;
  text.reserve(length + 1);
  for (size_t i = 0; i < length; ++i) {
    text += static_cast<char>(payload[i]);
  }

  const TransportCommandResponse response = handleTransportCommandStub(
      *gPortal, text.c_str(), TransportCommandSource::WebSocket, micros());
  String reply = response.body;
  gWsServer.sendTXT(clientNum, reply);
}

}  // namespace

/**
 * @brief Initializes portal HTTP/WebSocket transport routes and listeners.
 * @param portal Portal service instance backing request handling.
 * @par Used By
 * Main startup initialization.
 */
void initTransportRuntime(PortalService& portal) {
  if (gTransportInitialized) return;
  gPortal = &portal;

  gHttpServer.on("/api/v3/command", HTTP_POST, handleHttpCommandSubmit);
  gHttpServer.on(
      "/api/v3/diagnostics", HTTP_GET, []() {
        if (gPortal == nullptr) {
          gHttpServer.send(500, "application/json",
                           "{\"ok\":false,\"reason\":\"portal_not_ready\"}");
          return;
        }
        const PortalDiagnosticsState state = gPortal->diagnosticsState();
        if (!state.ready || state.json == nullptr || state.json[0] == '\0') {
          gHttpServer.send(503, "application/json",
                           "{\"ok\":false,\"reason\":\"diagnostics_not_ready\"}");
          return;
        }
        gHttpServer.send(200, "application/json", state.json);
      });
  gHttpServer.on("/api/v3/snapshot", HTTP_GET, handleHttpSnapshotGet);
  gHttpServer.begin();

  gWsServer.begin();
  gWsServer.onEvent(onWebSocketEvent);

  gTransportInitialized = true;
}

/**
 * @brief Services HTTP and WebSocket event loops.
 * @par Used By
 * Main loop.
 */
void serviceTransportRuntime() {
  if (!gTransportInitialized) return;
  gHttpServer.handleClient();
  gWsServer.loop();
}

}  // namespace v3::portal
