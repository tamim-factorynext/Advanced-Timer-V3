# API Contract V3

Date: 2026-02-28
Source Contract: `requirements-v3-contract.md` (v3.0.0-draft)
Related: `docs/schema-v3.md`, `docs/acceptance-matrix-v3.md`, `docs/decisions.md`
Status: Frozen for implementation

## 1. Scope

This document freezes transport-level payloads for runtime snapshots, runtime commands, and config lifecycle APIs.

## 2. Transport Model

- Runtime snapshots/events: WebSocket JSON messages.
- Runtime command path: WebSocket JSON command envelope.
- Config lifecycle path: HTTP JSON endpoints.
- Encoding: UTF-8 JSON.

## 3. Versioning

- API payloads use `apiVersion` (current: `2.0`).
- Config payloads embedded in API requests/responses use `schemaVersion` from `docs/schema-v3.md` (current: `2.0.0`).
- Unknown fields in responses/events must be ignored by clients for forward compatibility.

## 4. Common Metadata

All command results and mutating HTTP responses must include:

- `requestId` (string)
- `timestamp` (RFC3339 UTC)
- `status` (`SUCCESS|FAILURE`)
- `errorCode` (present on failure)
- `snapshotRevision` (where relevant)

## 5. WebSocket Contract

## 5.1 Runtime Snapshot Event

Message type: `runtime_snapshot`

```json
{
  "type": "runtime_snapshot",
  "schemaVersion": 1,
  "tsMs": 1740738600000,
  "snapshotSeq": 8123,
  "scanIntervalMs": 10,
  "lastCompleteScanMs": 0.812,
  "runMode": "RUN_NORMAL",
  "metrics": {
    "scanLastUs": 812,
    "scanMaxUs": 1240,
    "scanBudgetUs": 10000,
    "scanOverrunLast": false,
    "scanOverrunCount": 0,
    "queueDepth": 0,
    "queueHighWaterMark": 3,
    "queueCapacity": 16,
    "commandLatencyLastUs": 220,
    "commandLatencyMaxUs": 900
  },
  "testMode": {
    "active": false,
    "outputMaskGlobal": false,
    "breakpointPaused": false,
    "scanCursor": 0
  },
  "cards": [
    {
      "id": 8,
      "type": "DigitalOutput",
      "familyOrder": 8,
      "logicalState": true,
      "physicalState": true,
      "triggerFlag": false,
      "state": "State_DO_Active",
      "mode": "Mode_DO_Normal",
      "currentValue": 2,
      "startOnMs": 1740738600000,
      "startOffMs": 1740738600000,
      "repeatCounter": 0,
      "setResult": true,
      "resetResult": false,
      "maskForced": {
        "inputSource": "REAL",
        "forcedAIValue": 0,
        "outputMaskLocal": false,
        "outputMasked": false
      },
      "breakpointEnabled": false,
      "setResult": true,
      "resetResult": false,
      "evalCounter": 120,
      "debug": {
        "evalCounter": 120,
        "breakpointEnabled": false
      }
    }
  ]
}
```

Rules:
- `cards[]` order must match deterministic firmware evaluation order.
- Snapshot values are authoritative; clients must not recompute logical outcomes.
- Runtime snapshots must include `metrics` object fields defined in `docs/timing-budget-v3.md` Section 3.
- `metrics.scanBudgetUs` must equal `scanIntervalMs * 1000`.
- `metrics.queueDepth` must be `<= metrics.queueCapacity`.
- `cards[].evalCounter` is runtime-only metadata and must not be required in config commit payloads.

## 5.2 Command Request Envelope

Message type: `command`

```json
{
  "type": "command",
  "apiVersion": "2.0",
  "requestId": "cmd-0001",
  "name": "set_run_mode",
  "payload": { "mode": "RUN_NORMAL" }
}
```

Supported command names:
- `set_run_mode`
- `step_once`
- `set_breakpoint`
- `set_input_force`
- `set_output_mask`
- `set_output_mask_global`

## 5.3 Command Payload Definitions

`set_run_mode`:
```json
{ "mode": "RUN_NORMAL" }
```
- Allowed modes: `RUN_NORMAL|RUN_STEP|RUN_BREAKPOINT`.

`step_once`:
```json
{}
```

`set_breakpoint`:
```json
{ "cardId": 4, "enabled": true }
```

`set_input_force`:
```json
{ "cardId": 1, "mode": "REAL", "value": 0 }
```
- DI modes: `REAL|FORCED_HIGH|FORCED_LOW`.
- AI modes: `REAL|FORCED_VALUE` (requires numeric `value`).

`set_output_mask`:
```json
{ "cardId": 8, "masked": true }
```

`set_output_mask_global`:
```json
{ "masked": true }
```

## 5.4 Command Result Envelope

Message type: `command_result`

```json
{
  "type": "command_result",
  "apiVersion": "2.0",
  "requestId": "cmd-0001",
  "timestamp": "2026-02-26T10:30:00Z",
  "status": "SUCCESS",
  "errorCode": null,
  "message": null,
  "snapshotRevision": 8124
}
```

## 6. HTTP API Contract

## 6.1 Snapshot Read

`GET /api/snapshot`

Compatibility alias (active implementation):
- `GET /api/v3/snapshot`

Success response:
```json
{
  "type": "runtime_snapshot",
  "schemaVersion": 1,
  "tsMs": 1740738600000,
  "snapshotSeq": 8124,
  "scanIntervalMs": 10,
  "lastCompleteScanMs": 0.812,
  "runMode": "RUN_NORMAL",
  "metrics": {},
  "testMode": {},
  "cards": []
}
```

Rules:
- HTTP snapshot payload shape mirrors WebSocket `runtime_snapshot` payload.
- `snapshotSeq` returned by HTTP must be the latest complete snapshot revision at response time.

## 6.2 Config Lifecycle

### `GET /api/config/active`

```json
{
  "apiVersion": "2.0",
  "status": "SUCCESS",
  "timestamp": "2026-02-26T10:30:00Z",
  "activeVersion": "v42",
  "schemaVersion": "2.0.0",
  "config": {}
}
```

### `POST /api/config/staged/save`

Request:
```json
{
  "requestId": "cfg-1001",
  "apiVersion": "2.0",
  "schemaVersion": "2.0.0",
  "config": {}
}
```

Response:
```json
{
  "requestId": "cfg-1001",
  "apiVersion": "2.0",
  "status": "SUCCESS",
  "timestamp": "2026-02-26T10:30:01Z",
  "stagedVersion": "s19"
}
```

### `POST /api/config/staged/validate`

Request:
```json
{
  "requestId": "cfg-1002",
  "apiVersion": "2.0",
  "schemaVersion": "2.0.0",
  "config": {}
}
```

Response:
```json
{
  "requestId": "cfg-1002",
  "apiVersion": "2.0",
  "status": "SUCCESS",
  "timestamp": "2026-02-26T10:30:02Z",
  "validation": {
    "errors": [],
    "warnings": []
  }
}
```

### `POST /api/config/commit`

Request:
```json
{
  "requestId": "cfg-1003",
  "apiVersion": "2.0",
  "schemaVersion": "2.0.0",
  "config": {},
  "options": { "persist": true }
}
```

Response:
```json
{
  "requestId": "cfg-1003",
  "apiVersion": "2.0",
  "status": "SUCCESS",
  "timestamp": "2026-02-26T10:30:03Z",
  "activeVersion": "v43",
  "historyHead": {
    "lkgVersion": "v42"
  },
  "requiresRestart": false
}
```

### `POST /api/config/restore`

Request:
```json
{
  "requestId": "cfg-1004",
  "apiVersion": "2.0",
  "source": "LKG"
}
```

Allowed `source` values:
- `LKG`
- `FACTORY`

Response:
```json
{
  "requestId": "cfg-1004",
  "apiVersion": "2.0",
  "status": "SUCCESS",
  "timestamp": "2026-02-26T10:30:04Z",
  "restoredFrom": "LKG",
  "activeVersion": "v44",
  "requiresRestart": false
}
```

## 7. Error Model

Error object:

```json
{
  "errorCode": "VALIDATION_FAILED",
  "message": "set clause references unknown cardId",
  "field": "cards[3].config.set.clauseA.source.cardId",
  "details": { "cardId": 255 }
}
```

Stable error codes:
- `INVALID_REQUEST`
- `UNSUPPORTED_API_VERSION`
- `UNSUPPORTED_SCHEMA_VERSION`
- `VALIDATION_FAILED`
- `COMMIT_FAILED`
- `RESTORE_FAILED`
- `BUSY`
- `NOT_FOUND`
- `FORBIDDEN_IN_MODE`
- `UNAUTHORIZED`
- `FORBIDDEN`
- `INTERNAL_ERROR`

Rules:
- `errorCode` values are stable for automation.
- `message` is operator-facing and may evolve.
- `field` is optional and should be set for validation/form errors.

## 8. Compatibility Rules

- `RUN_SLOW` is not a valid V3 run mode and must be rejected by validation.
- Clients must tolerate additional fields in responses/events.
- Servers may reject unknown command names with `INVALID_REQUEST`.

## 9. Security Interaction

- Protected endpoints/commands require role-based authorization per `requirements-v3-contract.md` Section 16.
- Unauthorized authn state returns `UNAUTHORIZED`.
- Authenticated but insufficient role returns `FORBIDDEN`.

## 10. Acceptance Mapping

- `AT-API-001`: mutating response metadata.
- `AT-API-002`: validation failure payload shape.
- `AT-API-003`: latest complete snapshot retrieval.
- `AT-API-004`: WebSocket revision ordering.
- `AT-API-005`: global output mask command behavior.
- `AT-API-008`: runtime snapshot metrics object shape and required field presence.
- `AT-API-009`: `metrics.scanBudgetUs == scanIntervalMs * 1000` invariant.
- `AT-API-010`: queue depth invariant (`metrics.queueDepth <= metrics.queueCapacity`).
- `AT-CFG-006`: restore source constraints (`LKG|FACTORY`).



