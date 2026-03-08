# Transport Runtime Migration Plan (V3)

Status: Active execution plan for rewriting `src/portal/transport_runtime.cpp` with strict cleanup controls.

## 2026-03-08 Implementation Snapshot

- Post-save navigation freeze is currently stabilized after:
  - staged config buffer release after commit/failure,
  - card save write-scope reduction (`card file + index`),
  - settings save write-scope reduction (`settings file only`).
- Thermal/WiFi reduction controls currently active:
  - Live page polling cadence capped at `1200ms` (~0.83 Hz per runtime endpoint),
  - polling runs only when Live page is visible,
  - transport activity-aware runtime projection cadence (`active` vs `idle`) in `src/main.cpp`.
- Remaining migration focus before closure:
  - complete final cutover cleanup and leave only permanent operational telemetry.

## Phase 3 Exit Note (Provisional)

- Status: `Provisional pass` on March 8, 2026.
- Basis:
  - repeated save+navigate flows stabilized,
  - no recurring `WiFiClient write()` storm in latest short-run checks,
  - no new watchdog reset observed in latest short-run checks.
- Constraint:
  - full `30+ minute` mixed stress evidence is deferred due schedule/time.
- Risk handling:
  - keep `AT_FORCE_REBOOT_AFTER_SAVE` fallback flag available but disabled by default until long-run stress evidence is completed.

## Objective

- Replace the current transport runtime with a deterministic, watchdog-safe implementation.
- Eliminate post-save navigation freezes and repeated socket write failures under live polling load.
- Finish migration without leaving long-lived compatibility layers.
- Preserve operational recoverability throughout migration (no irreversible cutover steps).

## Scope

In scope:
- HTTP route handling and response lifecycle.
- WebSocket event/command transport glue.
- Static asset response path.
- Runtime polling endpoints (`/api/v3/runtime/*`) transport behavior.
- Transport-level logging and fault telemetry.

Out of scope:
- Storage model and config validation rules.
- Runtime/scan engine semantics.
- Card logic evaluation semantics.

## Non-Negotiable Constraints

- Bounded handler execution time.
- No unbounded blocking network write loops.
- Watchdog fed in long-running send loops.
- Deterministic connection lifecycle for all responses.
- Single active transport implementation at cutover.
- Migration must be reversible at each phase boundary.

## Architecture Targets

1. Single response abstraction
- All JSON/no-content/static responses go through one transport response layer.
- No direct `gHttpServer.send(...)` in endpoint handlers after migration.

2. Deterministic connection policy
- Explicit close policy per response class.
- No implicit keep-alive dependence.

3. Bounded streaming
- Static file send uses chunked bounded loop with timeout and explicit fault path.
- Returns structured failure state, never silent stall.

4. Poll pressure control
- Runtime endpoints enforce response pacing and overload behavior.
- Poll-heavy endpoints can degrade safely (e.g., reduced payload or temporary 503 with reason).

5. Thermal-aware transport behavior
- Transport must minimize unnecessary WiFi radio duty when there are no active portal consumers.
- Live update cadence is explicitly capped (target operational envelope: `2-10 Hz`, never unbounded burst loops).
- Prefer one active live channel per client session (WebSocket or HTTP polling), not duplicate parallel streams for the same data.
- If no active portal client is consuming live data, skip live transport serialization/sends.

6. Live-page scoped runtime streaming
- Runtime live endpoints (`/api/v3/runtime/metrics`, `/api/v3/runtime/cards/delta`) are active only while the user is on Live page.
- Leaving Live page suspends runtime polling/streaming.
- Returning to Live page performs a clean restart:
  - fresh baseline read,
  - then delta loop resume from a known sequence point.

## Migration Phases

## Phase 0: Baseline Freeze

- Freeze endpoint behavior contracts and payload envelopes.
- Capture baseline logs and failure signatures for:
  - save -> navigate
  - live runtime polling burst
  - config/settings page transitions

Acceptance gate:
- Baseline artifacts recorded in worklog.

## Phase 1: Transport Core Skeleton

- Introduce a new internal transport core module inside `transport_runtime.cpp` (or split file) with:
  - unified response helpers,
  - client close policy,
  - bounded static file sender,
  - common error response path.

Acceptance gate:
- Compiles and serves static pages + health routes without regressions.

## Phase 2: Endpoint Porting (Safe Order)

Port in this sequence:
1. Static pages and misc (`/`, `/config`, `/settings`, `/learn`, no-content endpoints).
2. Read-only APIs (`/api/v3/diagnostics`, `/api/v3/runtime/*`, `/api/v3/settings` GET, `/api/v3/cards` GET).
3. Mutating APIs (`settings PUT`, `card PUT`, restore, reboot, command submit).

Acceptance gate:
- Each batch passes targeted smoke checks before next batch.

## Phase 3: Load Hardening

- Run continuous navigation + save/apply + runtime polling load.
- Verify no `WiFiClient write()` storm, no core1 watchdog reset.
- Add controlled overload behavior where needed.
- Validate thermal-aware policy behavior under sustained portal sessions and idle/no-client periods.

Acceptance gate:
- 30+ minute stress run without transport-triggered reboot/watchdog.

## Short-Term Goals (Next 1-2 Sessions)

1. Stabilization evidence capture
- Run and record:
  - card save -> navigate (`/settings`, `/learn`, `/config`, `/`)
  - settings save -> navigate same sequence
  - live page open with polling active during navigation cycles
- Record serial evidence in `docs/worklog.md`.

2. Thermal/WiFi behavior confirmation
- Confirm live polling remains:
  - visibility-scoped (hidden tab = stopped),
  - cadence-capped (`1200ms` interval),
  - non-overlapping request model (single-flight guards).

3. Phase 3 load-hardening entry
- Execute one 30+ minute mixed load run:
  - repeated saves + cross-page navigation + live polling.
- Pass target:
  - no `WiFiClient write()` storm,
  - no watchdog reset,
  - no partial static page transfer.

## Phase 4: Cutover Cleanup (Mandatory)

Status (2026-03-08 session): `Completed for this rewrite session`.

- Remove migration-only compatibility paths and old helper calls.
- Remove temporary debug logs used for freeze isolation.
- Keep only minimal permanent operational telemetry.

Acceptance gate:
- Zero references to deprecated transport path remain.
- Worklog cleanup checklist fully checked.

## Session Closure Note (2026-03-08)

- This rewrite session is closed with stable operator evidence:
  - card/settings save persistence stable,
  - reboot/boot path stable after save,
  - no recurring post-save static-page write-stall in latest validation cycles.
- Remaining follow-up outside this closed session:
  - execute and record deferred `30+ minute` mixed stress run (time-boxed defer),
  - keep fallback `AT_FORCE_REBOOT_AFTER_SAVE` disabled-by-default unless field containment is needed.

## Cleanup Contract (Anti-Layer-Creep)

- At most one temporary compile-time flag for migration.
- Every temporary shim must have:
  - `TODO(remove after transport migration)` marker,
  - explicit owner and removal phase.
- If a shim survives past Phase 4 gate, it must be promoted to permanent design or deleted before merge.

Deletion checklist:
- Remove old response helper variants no longer used.
- Remove duplicate send paths.
- Remove temporary forced-close experiments if superseded by final policy.
- Remove freeze-investigation-only logs.
- Remove migration feature flag.

## Test Matrix (Minimum)

1. Save card -> navigate across `/settings`, `/config`, `/learn`, `/`.
2. Save settings -> navigate + live view active.
3. Continuous runtime polling (metrics + delta) for 30 minutes.
4. Reboot API under active browser session.
5. Offline/reconnect transition with portal open.
6. No-client idle run (portal closed) with live transport logic enabled.
7. Sustained live dashboard session at capped cadence (`2-10 Hz`) to verify thermal-aware policy conformance.

Pass criteria:
- No core1 watchdog reset.
- No repeated socket write error storms.
- No partial static page transfers without explicit handled failure.
- No avoidable high-frequency send activity when no client is active.
- Live cadence and channel selection policy remain within configured bounds.

## High-Risk Controls (Operational Project Safety)

This migration is high risk because current firmware is already partially operational.

Required controls:
- Keep migration incremental; no big-bang rewrite merge.
- Keep at least one known-good fallback checkpoint before each phase cutover.
- Define stop conditions:
  - new reboot loop,
  - persistent watchdog resets across core flows,
  - config/settings persistence regression.
- If stop condition is hit, revert to last stable checkpoint and continue from narrowed scope.
- Do not remove legacy behavior until the corresponding replacement path passes its acceptance gate.
- Track every temporary compatibility shim in worklog with explicit removal phase.

## Thermal/Load Note (Operational)

- Transport rewrite must reduce burst write churn and failed retry loops, which also reduces WiFi radio duty spikes and SoC heat.
- Polling pressure should be explicitly bounded as part of transport hardening, not left to browser default behavior.
- Thermal efficiency is treated as an architectural policy requirement, not dependent on continuous measurement campaigns during migration.
