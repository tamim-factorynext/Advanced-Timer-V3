# Dependency Topology Rules V3

Date: 2026-02-28
Source Contract: `requirements-v3-contract.md` (v3.0.0-draft)
Related: `docs/hardware-profile-v3.md`, `docs/fault-policy-v3.md`
Status: Draft for implementation

## 1. Purpose

Define allowed module dependencies so deterministic kernel execution remains isolated from portal, storage, and transport concerns.

## 2. Target Module Layers

- `src/kernel/`: card evaluation, scan loop, deterministic runtime state machine.
- `src/runtime/`: snapshot assembly, metrics, observability DTOs.
- `src/control/`: command validation/routing and runtime control semantics.
- `src/storage/`: staged config IO, commit/restore persistence, version metadata.
- `src/portal/`: HTTP/WebSocket handlers and payload translation.
- `src/platform/`: board pins, HAL adapters, RTOS wrappers, clock/watchdog integration.

## 3. Allowed Dependency Direction

Allowed direction is inward toward deterministic core:

`portal -> control -> runtime -> kernel`

`storage -> control -> runtime -> kernel`

`platform -> (all layers via interfaces only)`

## 4. Prohibited Dependencies

- `kernel` must not include or call:
  - web server / websocket classes
  - filesystem APIs
  - JSON serialization/deserialization
- `runtime` must not mutate kernel behavior as a side effect of snapshot generation.
- `portal` must not directly mutate kernel state; all writes go through command/config control interfaces.
- `storage` must not call portal/network code.

## 5. Interface Rules

- Cross-layer interaction must use explicit DTOs and function interfaces.
- Shared data exchanged across tasks must be copied/snapshotted, not exposed as mutable global pointers.
- Serialization formats (JSON) are boundary concerns (`portal` and `storage`), not kernel concerns.

## 6. Real-Time Safety Rules

- No dynamic heap allocation in kernel per-scan path.
- Any cross-core queue used by command/control path must have fixed capacity and telemetry.
- Snapshot publishing must be non-blocking to deterministic scan.

## 7. Enforcement

- New module/file dependencies must be reviewed against this document before merge.
- Violations must be tracked in `docs/decisions.md` with explicit remediation path or accepted exception.
