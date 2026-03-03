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

void initTransportRuntime(PortalService& portal);
void serviceTransportRuntime();

}  // namespace v3::portal
