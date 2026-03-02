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
  gHttpServer.begin();

  gWsServer.begin();
  gWsServer.onEvent(onWebSocketEvent);

  gTransportInitialized = true;
}

void serviceTransportRuntime() {
  if (!gTransportInitialized) return;
  gHttpServer.handleClient();
  gWsServer.loop();
}

}  // namespace v3::portal
