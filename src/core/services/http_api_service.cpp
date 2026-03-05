#include "core/services/http_api_service.hpp"

#include <Arduino.h>
#include <WiFi.h>

namespace at {
namespace services {

HttpApiService::HttpApiService(runtime::StatusSnapshotModel &statusModel)
    : statusModel_(statusModel), server_(80) {}

const char *HttpApiService::name() const {
  return "http_api";
}

void HttpApiService::init() {
  server_.on("/api/v4/status", HTTP_GET, [this]() { handleStatus(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void HttpApiService::tick(uint32_t /*nowMs*/) {
  ensureServerStarted();
  if (serverStarted_) {
    server_.handleClient();
  }
}

void HttpApiService::ensureServerStarted() {
  if (serverStarted_ || WiFi.status() != WL_CONNECTED) {
    return;
  }
  server_.begin();
  serverStarted_ = true;
  Serial.println("[HTTP] API server started on port 80");
}

void HttpApiService::handleStatus() {
  const runtime::SystemStatusSnapshot s = statusModel_.read();
  IPAddress ip(s.ip);

  char json[720];
  snprintf(json,
           sizeof(json),
           "{"
           "\"uptimeMs\":%lu,"
           "\"wifi\":{\"status\":%d,\"rssi\":%ld,\"ip\":\"%u.%u.%u.%u\"},"
           "\"core0\":{\"avgUs\":%lu,\"maxUs\":%lu,\"overruns\":%lu,\"loadPct\":%.2f},"
           "\"core1\":{\"avgUs\":%lu,\"maxUs\":%lu,\"overruns\":%lu,\"loadPct\":%.2f},"
           "\"memory\":{\"heapFree\":%lu,\"heapMin\":%lu,\"psramPresent\":%s,\"psramFree\":%lu,\"psramMin\":%lu},"
           "\"stack\":{\"core0Words\":%lu,\"core1Words\":%lu}"
           "}",
           static_cast<unsigned long>(s.uptimeMs),
           static_cast<int>(s.wifiStatus),
           static_cast<long>(s.wifiRssi),
           ip[0], ip[1], ip[2], ip[3],
           static_cast<unsigned long>(s.core0AvgUs),
           static_cast<unsigned long>(s.core0MaxUs),
           static_cast<unsigned long>(s.core0Overruns),
           s.core0LoadPct,
           static_cast<unsigned long>(s.core1AvgUs),
           static_cast<unsigned long>(s.core1MaxUs),
           static_cast<unsigned long>(s.core1Overruns),
           s.core1LoadPct,
           static_cast<unsigned long>(s.heapFree),
           static_cast<unsigned long>(s.heapMin),
           s.psramPresent ? "true" : "false",
           static_cast<unsigned long>(s.psramFree),
           static_cast<unsigned long>(s.psramMin),
           static_cast<unsigned long>(s.core0StackWords),
           static_cast<unsigned long>(s.core1StackWords));

  server_.send(200, "application/json", json);
}

void HttpApiService::handleNotFound() {
  server_.send(404, "application/json", "{\"error\":\"not_found\"}");
}

}  // namespace services
}  // namespace at

