/*
File: src/portal/routes.h
Purpose: Declares the routes module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/portal/routes.h

Flow Hook:
- Portal request handling and runtime snapshot transport.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <WebSocketsServer.h>

void initPortalServer();
void handlePortalServerLoop();

void initWebSocketServer();
void handleWebSocketLoop();
void publishRuntimeSnapshotWebSocket();
void handleWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload,
                          size_t length);

void handleHttpRoot();
void handleHttpSettingsPage();
void handleHttpConfigPage();
void handleHttpGetSettings();
void handleHttpSaveSettingsWiFi();
void handleHttpSaveSettingsRuntime();
void handleHttpReconnectWiFi();
void handleHttpReboot();
void handleHttpSnapshot();
void handleHttpCommand();
void handleHttpGetActiveConfig();
void handleHttpStagedSaveConfig();
void handleHttpStagedValidateConfig();
void handleHttpCommitConfig();
void handleHttpRestoreConfig();
