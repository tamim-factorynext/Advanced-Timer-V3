/*
File: src/portal/transport_runtime.h
Purpose: Declares the transport runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/main.cpp
Flow Hook:
- Portal request handling and runtime snapshot transport.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include "portal/portal_service.h"

namespace v3::portal {

/**
 * @brief Initializes HTTP/WebSocket transport endpoints for portal APIs.
 * @details Binds route handlers and websocket callbacks to the provided portal service.
 * @param portal Portal service instance used for command/snapshot/diagnostics operations.
 * @par Used By
 * - src/main.cpp
 */
void initTransportRuntime(PortalService& portal);
/**
 * @brief Services transport event loops for HTTP and WebSocket stacks.
 * @details Must be called from service loop to process ingress/egress traffic.
 * @par Used By
 * - src/main.cpp
 */
void serviceTransportRuntime();

}  // namespace v3::portal
