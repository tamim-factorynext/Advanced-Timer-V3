# V3 Portal Plan (Working Draft)

Date: March 3, 2026
Status: Foundation Stage (not final; evolving with design and contract decisions)

## 1. Intent And Maturity

This document is the portal planning baseline for V3.

Current reality:
- We are in early planning and contract-foundation mode.
- Scope and UX details are expected to evolve through structured iteration.
- This plan is intentionally progressive, not frozen.

How to use this document now:
- Treat it as the current best alignment across product vision, firmware constraints, and implementation direction.
- Update it whenever a decision is accepted after review.
- Avoid treating any section as final unless explicitly marked `Frozen`.

## 2. Product Vision

The Advanced Timer V3 portal is a field-ready no-code control interface for professional deployments.

The intended experience is:
- Safe for non-programmers to operate without external training.
- Clear under stress (alarms, overrides, unstable connectivity).
- Deterministic and trustworthy: runtime truth always comes from firmware snapshots.

### 2.1 Positioning

"Professional-Lite" means:
- Easier than PLC IDE workflows.
- Safer and more explicit than hobby dashboards.
- Structured for installation and maintenance realities.

### 2.2 Non-Negotiable UX Direction

- In-place help for key parameters and runtime controls.
- First-run guided path: connect -> inspect runtime -> edit staged config -> validate -> commit.
- Explicit labeling that prevents staged vs active confusion.

### 2.3 Drift Guard

- Do not add features only because competitors have them.
- Do not trade operational clarity for visual polish.
- Do not expand onboarding methods (for example BLE/Web Serial) unless release scope requires it.

## 3. Current Scope Baseline (Screens)

Current baseline is 4 pages:

1. **Live Runtime**
- Snapshot-driven states, alarms, overrides, metrics, and integrated debugging suite.

2. **Config Studio**
- Unified staged editor + validation results + commit/restore actions.

3. **Settings**
- WiFi user settings, theming/branding options, and operator-facing preferences.

4. **Introduction & Tutorial**
- Separate interactive first-use learning flow.

## 4. UX Invariants (Must Hold)

1. **Runtime truth is singular**
- Live values must come from firmware snapshot/event payloads.

2. **Active vs staged cannot be confused**
- Visual model and context must always separate both states.

3. **Dangerous actions are explicit**
- Force/mask/commit/restore must always show target and acknowledgement progression.

4. **Mobile-first usability**
- Touch-safe controls, high contrast status, and low typing friction.

5. **Action clarity by default**
- Available actions and guardrails are explicit; no ambiguous control states.

6. **Loading must feel alive**
- Blocking fetch/refresh paths show visible progress (skeleton/spinner/progress state).

7. **Offline and reconnect behavior is explicit**
- Connection state, reconnect progress, and stale-data context are always visible.

8. **Operational telemetry is built-in**
- Frontend emits structured reliability and action telemetry for diagnostics.

## 5. Technical Guardrails (ESP32 + Contract)

### 5.1 Contract Alignment

- Card order follows firmware deterministic scan order.
- Portal does not recompute authoritative runtime logic.
- Staged edits remain local until explicit validate/commit.
- Runtime controls reflect kernel-acknowledged state, not optimistic UI assumptions.
- Decimal <-> centiunit conversion is portal-side responsibility.

### 5.2 Platform Constraints

- Transport split:
- WebSocket for runtime stream.
- HTTP for config/validate/commit flows.
- Rendering budget target: `4-8 FPS` for live-card updates.
- Snapshot payload target: typical payload `<= ~12 KB`.
- Asset budget target: compressed portal bundle `<= ~300-400 KB` gzip.
- Reconnect behavior must follow explicit timeout/retry/backoff policy.

### 5.3 Current Security Scope

- Role-based access control is deferred in this phase.
- Even without RBAC, dangerous actions must remain explicit and auditable.

## 6. Implementation Roadmap (Progressive, Page-First)

Execution rule:
- Build pages in the agreed order with minimal viable behavior first.
- End every phase with one demoable vertical slice (UI + API wiring + loading/error handling).

### Phase 1: Foundation Shell

Build:
- Bootstrap-first MPA scaffold with progressive enhancement.
- Top navigation and page routing skeleton for all 4 pages.
- Shared status bar shell, loading/refresh primitives, and error state patterns.
- Typed HTTP/WebSocket client baseline and DTO guards.
- Connectivity model: `CONNECTED`, `DEGRADED`, `OFFLINE`, `RECONNECTING`.
- Telemetry baseline events:
- lifecycle (`app_loaded`, `route_opened`)
- transport (`ws_connected`, `ws_disconnected`, `ws_reconnect_attempt`, `api_timeout`)
- actions (`command_submitted`, `command_acked`, `validate_started`, `validate_failed`, `commit_started`, `commit_result`)

Exit gate:
- All pages are reachable from nav with consistent shell behavior.
- Core app boots against firmware endpoint and mock endpoint.
- No hard-stall loading behavior.

### Phase 2: Live Runtime (Basic)

Build:
- Basic homepage with placeholders for upcoming live modules.
- Card physical status strip with simple green/red indicator where applicable.
- Running clock and basic device health summary.
- Navigation actions to all other pages.

Exit gate:
- Snapshot-driven status indicators render reliably.
- Live page shows deterministic loading, empty, and error states.
- No ambiguous stale-data presentation during disconnect.

### Phase 3: Settings (Easy-Win Delivery)

Build:
- WiFi user settings and basic operator preferences.
- Branding/theming baseline controls within readability limits.
- Save/apply feedback with clear success/failure reason messaging.

Exit gate:
- Settings round-trip is reliable with validation feedback.
- Rejections and failures are visible and auditable in telemetry.

### Phase 4: Config Studio

Build:
- Unified flow: active load -> staged edit -> validate -> commit/restore.
- Clear staged vs active separation throughout UI.
- Offline mutation safeguards and reconnect recovery behavior.
- Family editor progression for `DI`, `AI`, `SIO`, `DO`, `MATH`, `RTC`.
- Binding support (`CONSTANT`, `VARIABLE_REF`) with compatibility guards.

Exit gate:
- Pass target acceptance anchors: `AT-UI-002`, `AT-UI-003`, `AT-UI-004`, `AT-API-001`, `AT-API-002`, `AT-API-003`.
- Pass target acceptance anchors: `AT-DATA-001`, `AT-DATA-003`, `AT-BIND-001..006`.
- No silent command/config loss across reconnect events.

### Phase 5: Introduction & Tutorial

Build:
- Separate interactive tutorial page connected from first-run and settings/help entry.
- Guided path for runtime basics, staged vs active model, validate, and commit.
- Context help linking from tutorial into real pages.

Exit gate:
- First-run user can complete commissioning flow without external documentation.
- Tutorial completion path is measurable and stable on mobile.

### Phase 6: Live Runtime (Advanced Debug + Simulation)

Build:
- Full live debugging suite on the Live Runtime page:
- transport status
- queue depth/high-water/drop
- timing context and latest diagnostics
- Simulation/force interactions polished as final-stage features.
- Telemetry and diagnostics correlation for incident replay.

Exit gate:
- Advanced debug/simulation tools operate without degrading core runtime UX.
- Mobile smoke tests pass for core workflows.
- At least one disconnect/reconnect incident can be explained end-to-end through telemetry traces.

## 7. Frontend Stack Adoption Plan (Incremental)

Adopt one stack at a time in this order:

1. `Bootstrap 5`
2. `Bootstrap Icons`
3. `Alpine.js`
4. `HTMX`
5. `Chart.js` (optional, high-value use only)

Adoption policy:
- Start with CDN assets in development for fast trial and rollback.
- Add only one new stack per iteration cycle.
- Measure impact after each addition:
- UX gain
- responsiveness
- network behavior
- asset size delta
- Promote only proven additions.
- Before release, pin versions and move approved assets to local LittleFS hosting.

## 8. Traceability Snapshot

| Objective | Anchor | Acceptance Reference |
| --- | --- | --- |
| Operational awareness | Sec 14.1 | `AT-UI-001` |
| Runtime trust | Sec 14.2 | `AT-UI-002`, `AT-UI-003` |
| Staged/active clarity | Sec 14.2 | `AT-UI-004` |
| Command acknowledgement integrity | Sec 15.1, 15.2 | `AT-API-001`, `AT-API-005` |
| Numeric correctness at UI boundary | Sec 4.1, 4.2 | `AT-DATA-001`, `AT-DATA-003` |
| Offline/reconnect clarity | Sec 14.2, Sec 15.2 | `AT-API-004`, `AT-UI-003` |
| Portal diagnostics quality | Sec 15.2 | `AT-API-008` |
| Fresh V3 UI baseline | `DEC-0019` | Delivery review checkpoint |

## 9. Out Of Scope For Current Phase

- Remote/cloud multi-device fleet management.
- Full white-label template marketplace.
- BLE/Web Serial onboarding unless explicitly pulled into release scope.
- Role-based access control (`VIEWER`/`OPERATOR`/`ENGINEER`/`ADMIN`) and session-policy enforcement.

## 10. Working Method (How We Will Evolve This Plan)

- Run focused brainstorming sessions twice per week:
- Session A: UX flow and operator clarity.
- Session B: technical feasibility and contract fit.
- For each session:
- capture candidate ideas,
- score by safety, clarity, and ESP32 cost,
- promote only accepted items into this document.

Change discipline:
- Use small, explicit updates over large ambiguous rewrites.
- Mark assumptions and unresolved items clearly.
- Re-check coherence after each major update.

## 11. Provisional Definition Of Ready (Before Full Implementation)

Start large-scale implementation only when:

1. Screen model and page boundaries are stable for at least one review cycle.
2. Runtime/config/command payload contracts are stable enough for typed client work.
3. Baseline stack rollout strategy is agreed for first two dependencies.
4. Acceptance anchors for current phase are mapped and review-approved.

## 12. Open Questions (Working Backlog)

These questions are intentionally unresolved and should be reviewed in upcoming brainstorming sessions before implementation lock.

1. Live Runtime basics:
- Which exact card fields must appear in Phase 2 basic status rows (minimum useful set)?
- Should health summary show only firmware-provided fields or include portal-derived convenience indicators?

2. Config Studio scope boundary:
- In first Config Studio pass, should all card families ship together or in two waves (for example `DI/DO` first, then others)?
- How much inline validation help text is needed in-v1 versus deferred tooltip/help expansion?

3. Settings boundary:
- Which preferences are strictly in-scope for first pass beyond WiFi (timezone, refresh interval, theme preset)?
- What settings should remain hidden until after tutorial completion?

4. Tutorial strategy:
- Is tutorial strictly linear or can users jump sections while still preserving completion tracking?
- Should tutorial progress persist locally in browser only, or sync to device profile later?

5. Debug + simulation final stage:
- Which simulation controls are mandatory for first release (force input, run mode control, mask)?
- What guardrails/confirmations are mandatory before exposing simulation actions in Live Runtime?

6. Asset and stack rollout:
- What exact per-phase asset-size budget should be enforced to protect LittleFS headroom?
- At what threshold should `Chart.js` be rejected or deferred in favor of simpler visuals?
