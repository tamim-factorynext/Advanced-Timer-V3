# V3 Portal Plan (Working Draft)

Date: March 4, 2026
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
- Structure is three-part:
- `Introduction`: interactive presentation-style core concept briefing (not a literal tutorial, minimal direct interface referencing).
- `Tutorial`: step-by-step interaction guide for key portal elements (complements guided tour).
- `Curated Examples`: practical example collection that helps users form mental models for custom programs/behaviors.

## 4. UX Invariants (Must Hold)

1. **Runtime truth is singular**
- Live values must come from firmware snapshot/event payloads.

2. **Active vs staged cannot be confused**
- Visual model and context must always separate both states.

3. **Dangerous actions are explicit**
- Force/mask/commit/restore must always show target and acknowledgement progression.

4. **Mobile-first usability**
- Touch-safe controls, high contrast status, and low typing friction.
- Bottom safe area and on-screen keyboard behavior must never obscure critical controls.

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
- Current ESP32 LittleFS partition ceiling: `~1.4 MB` maximum.
- If asset/storage budget pressure blocks delivery on current ESP32, immediately start migration phase to `ESP32-S3 N16R8`.
- Reconnect behavior must follow explicit timeout/retry/backoff policy.

### 5.3 Current Security Scope

- Role-based access control is deferred in this phase.
- Even without RBAC, dangerous actions must remain explicit and auditable.

## 6. Implementation Roadmap (Progressive, Page-First)

Execution rule:
- Build pages in the agreed order with minimal viable behavior first.
- End every phase with one demoable vertical slice (UI + API wiring + loading/error handling).

### 6.1 Delivery Model (Locked For This Phase)

- Use iterative collaborative frontend + backend delivery (simultaneous work, not frontend-last).
- Implement in vertical slices:
- each slice includes contract update (if needed), backend endpoint wiring, frontend integration, loading/error states, and hardware validation.
- No mock API phase for primary flows. Build and integrate against live firmware endpoints slice-by-slice.
- Avoid large one-shot merges; prefer small reviewable increments with immediate hardware verification.

### Phase 1: Foundation Shell

Build:
- Bootstrap-first MPA scaffold with progressive enhancement.
- Top navigation and page routing skeleton for all 4 pages.
- Iconography baseline applied across all pages from initial delivery.
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
- Minimum basic status row includes:
- `DI`, `DO`, `Virtual Output`: physical state.
- `AI`: live value.
- `MATH`: live value.
- `RTC`: logical state.
- Running clock and basic device health summary:
- Use firmware-provided fields as baseline.
- Include essential portal-derived convenience indicators.
- Navigation actions to all other pages.

Exit gate:
- Snapshot-driven status indicators render reliably.
- Live page shows deterministic loading, empty, and error states.
- No ambiguous stale-data presentation during disconnect.

### Phase 3: Settings (Easy-Win Delivery)

Build:
- Full first-pass coverage of supported settings in one release wave:
- WiFi
- timezone
- time server
- stepping interval slider
- theme presets (minimum 5 in first release):
- 1 light theme
- 1 dark theme
- 1 solarized-style theme
- 2 colorful themes
- reboot
- simulation-controls enable preference (browser-side only; default OFF)
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
- All card families ship together in first pass: `DI`, `AI`, `SIO`, `DO`, `MATH`, `RTC`.
- Binding support (`CONSTANT`, `VARIABLE_REF`) with compatibility guards.
- Inline validation help text:
- Ship essential help in first pass.
- Defer non-essential guidance expansion to later refinement.

Exit gate:
- Pass target acceptance anchors: `AT-UI-002`, `AT-UI-003`, `AT-UI-004`, `AT-API-001`, `AT-API-002`, `AT-API-003`.
- Pass target acceptance anchors: `AT-DATA-001`, `AT-DATA-003`, `AT-BIND-001..006`.
- No silent command/config loss across reconnect events.

### Phase 5: Introduction & Tutorial

Build:
- One dedicated learning area connected from first-run and settings/help entry, containing:
- `Introduction`: interactive game-style conceptual walkthrough.
- `Tutorial`: strict linear step-by-step flow with skip option.
- `Curated Examples`: capability demonstrations to guide mental model formation.
- Tutorial remains separate from (and complementary to) guided in-app tour.
- Tutorial content should not over-reference the production UI in the Introduction segment.
- Tutorial progress is non-persistent in first release.

Exit gate:
- First-run user can complete commissioning flow without external documentation.
- Tutorial completion path is measurable and stable on mobile.

### Phase 6: Live Runtime (Advanced Debug + Simulation)

Build:
- Full live debugging suite on the Live Runtime page:
- transport status
- queue depth/high-water/drop
- timing context and latest diagnostics
- Run-mode controls included in first release of debug/simulation suite:
- stepping
- continuous
- breakpoints
- Breakpoint control and clear behavior indicators are mandatory first-pass features.
- Simulation/force controls are hidden by default and become visible only when enabled from Settings.
- Telemetry and diagnostics correlation for incident replay.
- Live Runtime Card Wizard:
- per-card `Edit` action opens guided card editor popup.
- wizard supports full per-card parameter surface including set/reset condition rules.
- wizard visibility is gated by `debugModeEnabled=true` only.
- finish action performs explicit save + reboot flow to apply changes.
- Live card badge rendering contract:
- every card shows static parameter badges continuously.
- every card shows compact one-line `SET` and `RST` expression badges continuously.
- combiner display is expression-based only (`A`, `A & B`, `A | B`) with no explicit `[AND]/[OR]/[NONE]` tag.
- expression token vocabulary is fixed:
- `commandState=ON/OFF`, `actualState=HIGH/LOW`, `edgePulse=TRIG/CLR`, `missionState=IDLE/RUN/DONE`, constant clauses as `CardRef:TRUE/FALSE`.
- badge color (not text) indicates runtime evaluation result.
- compact badge text style is fixed for operator readability:
- use `KEY:VALUE` with no space after `:`.
- prefer short keys on live cards (for example `SIG`, `TRG`, `DEB`, `CMD`, `PHYS`, `EDGE`, `COUNT`).
- DI reference notation:
- `SIG:NORM|INV`, `TRG:RISE|FALL|CHG`, `DEB:<seconds>s`, runtime row `CMD:<...>`, `PHYS:<...>`, `EDGE:<...>`, `COUNT:<...>`.
- Live card layout contract (strict 4-zone):
- Zone 1: header with icon + family title + logical terminal identity (`DI0/DO1/...`) + dominant live-state indicator.
- Zone 1 body badges should avoid redundant technical identity fields (`DIx`, `ID`, `CH`) for operator-facing Live view.
- Zone 2: primary runtime signals (family-specific compact row).
- Zone 3: set/reset logic strip (families with set/reset support only).
- Zone 4: debug actions area with left-aligned force-group controls (`LOW | REAL | HIGH`) and right-aligned `Edit` action.
- Zone 4 should not include extra debug indicator or command-status/result badges.
- Zone 4 is debug-gated:
- `debugModeEnabled=false` -> Zone 4 is not rendered and consumes no card space.
- `debugModeEnabled=true` -> Zone 4 is visible.
- Live card grid baseline:
- desktop: 3 columns
- tablet: 2 columns
- mobile: 1 column
- Disabled-card display policy:
- keep Zones 2 and 3 visible in muted inactive style for context stability.
- show configured set/reset expressions but suppress active evaluation coloring.
- show `DISABLED` in header state indicator.
- Live badge data-source policy (resource-safe):
- do not expand runtime card payload with static condition-expression text.
- perform one-time static bootstrap from config APIs (`GET /api/v3/cards` + `GET /api/v3/cards/{id}`).
- use runtime APIs (`/api/v3/runtime/metrics`, `/api/v3/runtime/cards/delta`) only for dynamic updates and badge evaluation state/color.
- refresh static cache after save/reboot or when `activeVersion` changes.

Exit gate:
- Advanced debug/simulation tools operate without degrading core runtime UX.
- Mobile smoke tests pass for core workflows.
- At least one disconnect/reconnect incident can be explained end-to-end through telemetry traces.

### 6.2 DI Wizard UX Blueprint (Operator + Learning)

Goal:
- Make Live `Edit` wizard a guided commissioning flow for average users, not a compact clone of Config Studio.
- Teach runtime behavior while user configures the card.

Wizard information model (must be explicit in UI):
- `Editable`: controls users can change.
- `Informational`: plain read-only facts (never disabled form inputs).
- `Educational`: concise “what this means” helper text + example.

Non-editable field rule:
- Render as info rows/chips only.
- Do not render read-only card identity/channel facts as disabled inputs.

DI step flow target:
1. Context:
- card identity, current runtime state, enabled/disabled.
- one-line mental model: DI converts electrical input into logical events.
2. Signal behavior:
- `SIG`, `TRG`, `DEB` controls.
- include live behavior sentence preview (example: “Input must remain stable for 0.05s before rising edge counts.”).
3. SET condition:
- title language: “Turn ON when...”
- full clause editor + plain-language examples.
4. RST condition:
- title language: “Turn OFF when...”
- same editor pattern; explicitly explain reset-priority impact.
5. Review + Save:
- before/after summary,
- warning panel,
- explicit `Save & Reboot`.

Learning content (must align with user guide):
- processing order:
- source select -> invert -> actualState -> set/reset -> gating -> edge/debounce -> counter/pulse.
- debounce behavior:
- `debounceMs=0` immediate, `>0` requires full stable window.
- force behavior:
- force applies before invert.
- gating behavior:
- `reset=true` can block qualification in that scan.

Badge language parity rule:
- Wizard explanation text must match Live badges.
- Keep compact badge style `KEY:VALUE` (no space after `:`), including DI baseline:
- `SIG:NORM|INV`, `TRG:RISE|FALL|CHG`, `DEB:<seconds>s`, `CMD:<...>`, `PHYS:<...>`, `EDGE:<...>`, `COUNT:<...>`.

Responsive contract:
- Desktop: two-pane step layout (edit left, explanation/preview right).
- Mobile: single-column stacked flow (edit first, explanation under it), no horizontal scroll.
- Keep action buttons stable and reachable on both form factors.

Safety UX:
- prefer prevention over post-submit errors.
- show inline validation for local field issues.
- show step-level warnings for risky logic combinations.
- block save on contract-invalid condition shapes.

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
- `Bootstrap Icons` is required from the beginning and should not be deferred beyond initial shell delivery.
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

- Replace twice-weekly brainstorming cadence with one concentrated marathon alignment session to finish planning lock.
- During the marathon session:
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

These questions are intentionally unresolved and should be reviewed in the marathon alignment session before implementation lock.

1. Asset and stack rollout:
- What exact per-phase asset-size budget should be enforced to protect LittleFS headroom for current ESP32 target?
- At what threshold should `Chart.js` be rejected or deferred in favor of simpler visuals?

2. Card editing UX model (Live vs Config split):
- Dual-mode approach is accepted:
- `Config Studio` remains full expert editor.
- `Live Runtime` includes guided `Card Wizard` for per-card commissioning edits in debug mode.
- Full cross-card topology/binding editing boundaries remain Config Studio-owned unless explicitly promoted in future decision.
- What diff/confirmation model is mandatory to prevent confusion when edits can originate from both pages?
- What synchronization signals must always be visible (`activeVersion`, last change source, last change timestamp) after any commit?

Decisions already accepted above were removed from this backlog to keep Section 12 strictly unresolved.
