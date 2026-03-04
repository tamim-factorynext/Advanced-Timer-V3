/*
File: src/portal/legacy_routes.h
Purpose: Declares the routes module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- firmware build target
Flow Hook:
- Portal request handling and runtime snapshot transport.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <WebSocketsServer.h>

void legacyInitPortalServer();
void legacyHandlePortalServerLoop();

void legacyInitWebSocketServer();
void legacyHandleWebSocketLoop();
void legacyPublishRuntimeSnapshotWebSocket();
void legacyHandleWebSocketEvent(uint8_t clientNum, WStype_t type,
                                uint8_t* payload, size_t length);

void legacyHandleHttpRoot();
void legacyHandleHttpSettingsPage();
void legacyHandleHttpConfigPage();
void legacyHandleHttpGetSettings();
void legacyHandleHttpSaveSettingsWiFi();
void legacyHandleHttpSaveSettingsRuntime();
void legacyHandleHttpReconnectWiFi();
void legacyHandleHttpReboot();
void legacyHandleHttpSnapshot();
void legacyHandleHttpCommand();
void legacyHandleHttpGetActiveConfig();
void legacyHandleHttpStagedSaveConfig();
void legacyHandleHttpStagedValidateConfig();
void legacyHandleHttpCommitConfig();
void legacyHandleHttpRestoreConfig();

