# Advanced Timer V3 - Single Master Build Prompt

Use this prompt verbatim with a highly capable coding model to rebuild the project from scratch while preserving critical intent.

---

You are a senior embedded + full-stack systems engineer. Build a production-grade "Advanced Timer V3" platform from scratch.

## Mission

Build an ESP32 deterministic control firmware + web portal with strict contract-driven behavior.

The product target is "Professional-Lite":
- No-code workflow for non-PLC users.
- Deterministic runtime semantics suitable for industrial-style use.
- Strong safety guardrails: staged validation/commit, rollback, role-based control, auditable actions.

## Product Boundaries

### In scope
- Deterministic logic engine on ESP32.
- Six card families: `DI`, `AI`, `SIO`, `DO`, `MATH`, `RTC`.
- HTTP + WebSocket API.
- Local web portal UI for runtime, config, maintenance, settings.
- Config schema, validation, lifecycle, persistence, rollback.
- Security role model and audit trail.
- Contract-linked test suite (unit + integration + HIL-ready).

### Out of scope for v3.0
- Cloud fleet management.
- Feature additions based on competitor parity only.
- BLE/Web Serial onboarding unless explicitly enabled by stable firmware support.
- Any hidden execution path that bypasses deterministic core rules.

## Non-Negotiable Architecture

### Two-core ownership model
- `Core0` deterministic core:
  - scan loop
  - card evaluation
  - deterministic runtime transitions
  - bounded-time command application
- `Core1` networking/non-deterministic core:
  - portal HTTP/WebSocket
  - WiFi policy
  - persistence workflows
  - diagnostics/services/logging

No direct mutation of deterministic runtime state from portal/networking code.
All interactions must pass through explicit control/data contracts.

### Layer boundaries
Use explicit modules with dependency direction like:
`portal -> control -> runtime -> kernel`
`storage` and serialization stay boundary concerns; kernel must be transport-agnostic.

## Determinism and Runtime Contract

- Deterministic card execution order is fixed by ascending `cardId`.
- Exactly one immutable snapshot revision per completed scan.
- No dynamic heap allocations in per-scan kernel path.
- Run modes supported: `RUN_NORMAL`, `RUN_STEP`, `RUN_BREAKPOINT`.
- No `RUN_SLOW` mode.
- Runtime truth is authoritative only from snapshots/events produced by firmware.

## Data and Validation Contract

### Numeric representation
- Portal-facing numbers may be decimal.
- Firmware API/storage uses unsigned fixed-point centiunits where required (`x100`).
- Portal must encode/decode decimal <-> centiunits correctly.
- Negative values in unsigned fields must be rejected by validation.

### Family model
Implement typed config/runtime handling for all families:
- `DI`: debounce, edge qualification, invert ordering, force modes.
- `AI`: pipeline `raw -> clamp -> map/scale -> EMA`, quality flags, forced value validation.
- `DO`: mission-based behavior, set/reset precedence, mask semantics, repeat logic, retrigger rules.
- `SIO`: DO-like mission semantics without physical GPIO drive; mask unsupported.
- `MATH`: standard arithmetic pipeline (`ADD|SUB_SAT|MUL|DIV_SAFE`), input clamp -> scaling (inverse allowed) -> EMA, fallback/hold behavior.
- `RTC`: minute-level schedule matching, duration windows, no second/millisecond schedule precision.

### Set/reset shared contract
- Implement common set/reset condition block with explicit per-family support matrix.
- Reject unsupported clauses/fields/operators by family.
- `missionState` source rules must be strict (`DO/SIO` only, `EQ` only).

### Variable binding contract
Support:
- `CONSTANT`
- `VARIABLE_REF`

Validation must reject:
- type mismatch
- range/unit mismatch
- unsupported state source
- cycles/forward dependency loops

Commit must be atomic: if any binding invalid, no partial apply.

## Config Lifecycle and Persistence

Lifecycle states:
- `ACTIVE`
- `STAGED`
- `VALIDATED`
- `COMMITTED`

Endpoints/workflow:
- load active
- save staged
- validate staged
- commit staged
- restore from allowed sources

Persistence/safety requirements:
- transactional commit protocol
- power-loss-safe behavior
- preserve last-known-good rollback target
- allow restore only from approved sources (`LKG`, `FACTORY`)

Use versioned config envelope with explicit `apiVersion` and `schemaVersion` validation.

## WiFi Contract

- Dual-SSID strategy: master SSID short timeout, user SSID long timeout.
- If neither available, enter offline mode while kernel remains operational.
- STA-only mode; AP mode disabled in production path.
- Network load must not break deterministic scan budget.

## API Contract

Implement at minimum:
- `GET /api/snapshot`
- command endpoint for runtime controls
- `GET /api/config/active`
- `POST /api/config/staged/save`
- `POST /api/config/staged/validate`
- `POST /api/config/commit`
- `POST /api/config/restore`

API guarantees:
- machine-readable error codes + human message
- response metadata includes `requestId`, `timestamp`, `status`
- forward compatibility with unknown fields
- WebSocket stream preserves increasing revision ordering

## Security and Audit

Role model:
- `VIEWER`
- `OPERATOR`
- `ENGINEER`
- `ADMIN`

Protected operations must enforce role permissions.
Every protected success/failure action must produce audit records with actor, role, action, result, and context.

## Portal UX Contract (critical)

Build a fresh V3 portal baseline (do not patch legacy UI).

Required UX invariants:
1. Fixed header always visible with run mode, override status, alarm state, connectivity.
2. Card rendering order matches firmware scan order.
3. UI never recomputes authoritative runtime logic values.
4. Active vs staged config state is impossible to confuse.
5. Runtime controls show acknowledged kernel state, not optimistic local state.
6. Mobile-first operational usability (touch targets, high contrast, low typing burden).
7. Self-explanatory UX: in-place help for all parameters; guided first-run flow.

Primary portal screens:
- Live Runtime
- Configuration Workspace
- Validation/Commit
- Maintenance Panel (diagnostics/health/recovery)
- Settings/Access (WiFi, roles/session, theme tokens)

## Observability, Faults, and Performance

Expose runtime metrics in snapshot, including at least:
- scan timing budget and observed timing
- queue depth/capacity
- per-card eval duration (`lastEvalUs`)
- counters/reboot reason as required by reliability policy

Fault model:
- classed severity (`INFO`, `WARN`, `CRITICAL`)
- safe-state behavior for unsafe outputs
- degraded mode policy that protects deterministic core

Performance invariants:
- deterministic scan cadence stability at configured intervals (including 10 ms target)
- bounded snapshot generation overhead
- command/API load does not violate core timing constraints

## Testing and Acceptance Strategy

Create a traceable acceptance matrix and implement tests for:
- core determinism and topology rules
- centiunit conversion and numeric validation
- each card-family behavior contract
- variable binding rules and cycle rejection
- config lifecycle atomicity and rollback safety
- API payload guarantees and WebSocket ordering
- role enforcement and audit logging
- WiFi behavior and scan stability under load
- fault policy and degraded-mode behavior

Use three tiers:
- fast native/unit tests
- firmware integration tests
- hardware-in-the-loop smoke (Pi-masterable rig)

Treat acceptance gates as release blockers for critical tests.

## Implementation Constraints

- Language/tooling baseline: ESP32 Arduino framework with PlatformIO.
- Filesystem: LittleFS for portal assets and/or config artifacts.
- Keep code modular; avoid monolithic `main.cpp` ownership of all concerns.
- Prefer typed models and explicit adapters/bridges over untyped JSON passthrough.
- Make migration seams explicit if legacy compatibility is temporarily needed.

## Required Deliverables

1. Buildable firmware project with clear module layout.
2. Contract docs: schema, API, hardware profile, timing budget, fault policy, acceptance matrix, decisions.
3. Web portal codebase aligned to UX invariants above.
4. Test suites and scripts for native/integration/HIL-friendly execution.
5. Seed configs and example payloads for all card families.
6. Concise engineering README describing architecture and run/test workflow.

## Output Format Required From You (the model)

Produce work in this order:
1. Architecture map and module boundaries.
2. Data contracts (schemas, DTOs, enums, validation rules).
3. Runtime engine implementation.
4. API + persistence workflow implementation.
5. Portal implementation.
6. Test implementation and acceptance traceability.
7. Final verification report listing which gates pass/fail.

For each stage, include:
- files created/modified
- rationale tied to the contract
- explicit assumptions
- remaining risks

Do not skip difficult areas by leaving TODO placeholders for contract-critical behavior.
If uncertainty exists, choose the safer deterministic interpretation and document it.

---

Optional project-specific addendum you can prepend before running this prompt:
- Board/profile capacities and concrete per-family channel counts.
- Branding/theming preferences.
- Exact role-permission matrix for protected ops.
- Manufacturing/HIL station constraints.
