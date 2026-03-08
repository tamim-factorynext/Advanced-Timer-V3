# V3 Portal Execution Board

Date: 2026-03-06  
Purpose: Convert the broad portal plan into a small, executable sequence with tight scope.

## 1. Current Reality

- Backend transport and core API routes already exist (`/api/v3/runtime/*`, `/api/v3/settings`, `/api/v3/cards`, `/api/v3/command`).
- Frontend pages exist in `data/` (`index.html`, `config.html`, `settings.html`, `learn.html`) and are directly served by firmware.
- Main risk now is not architecture; it is scope sprawl and too many parallel tasks.

## 2. Rules To Reduce Overload

- Keep max 1 in-progress portal slice at a time.
- Every slice must be demoable on device, not only in code.
- No new feature starts until current slice has:
- API contract check
- mobile smoke check
- one failure-path check
- If a task takes more than 2 days, split it into two smaller deliverables.

## 3. Next 4 Slices (Do In Order)

## Slice P1: Runtime Page Stability Pass

Goal:
- Make Live page robust under disconnect/reconnect and stale-data windows.

Scope:
- Harden `/api/v3/runtime/cards/delta` consumption on frontend.
- Add explicit UI state badges: `ONLINE`, `RECONNECTING`, `OFFLINE`, `STALE`.
- Add retry backoff visualization.

Done when:
- Unplug/replug WiFi test shows no frozen UI.
- Card list resync works after missed sequences.
- No ambiguous "looks live but stale" state.

## Slice P2: Settings Reliability Pass

Goal:
- Make settings flow predictable and auditable.

Scope:
- Tighten request/response handling for `GET/PUT /api/v3/settings`.
- Show field-level validation feedback in UI.
- Add explicit `requiresRestart` handling message and action hint.

Done when:
- Invalid payload gives user-visible reason.
- Successful save always shows active revision and restart requirement.
- Mobile form interaction stays usable with keyboard open.

## Slice P3: Config Studio Thin Vertical Slice

Goal:
- Ship minimal but real config editing path first.

Scope:
- Build cards index panel from `GET /api/v3/cards`.
- Build single-card edit/load/save loop via `GET/PUT /api/v3/cards/{id}`.
- Start with DI + AI edit support only.

Done when:
- Operator can edit one DI card and one AI card end-to-end.
- Validation errors round-trip to UI.
- Commit failure path is handled with clear message.

## Slice P4: Command UX + Diagnostics Coupling

Goal:
- Make run-mode/step/force actions traceable from click to kernel apply.

Scope:
- Wire command actions to `/api/v3/command` with visible request IDs.
- Show latest command result + reason in runtime panel.
- Surface selected diagnostics counters for command parity checks.

Done when:
- Operator can trigger `setRunMode` and `stepOnce` from UI.
- Request ID from submit is visible in UI and matched in diagnostics data.
- Queue-full and reject reasons are visible without logs.

## Slice P4B: Live Badge Rendering Baseline

Goal:
- Lock and implement compact always-visible card badges before wizard editing behavior.

Scope:
- Render static parameter badges on every live card.
- Render dynamic one-line `SET` and `RST` expression badges on every live card.
- Build static badge-expression cache via one-time config bootstrap:
- `GET /api/v3/cards` then per-card `GET /api/v3/cards/{id}`.
- Keep runtime polling limited to:
- `/api/v3/runtime/metrics`
- `/api/v3/runtime/cards/delta`
- Do not add static badge expression text fields into runtime payload.
- Apply frozen expression format:
- primary-only -> `A`
- AND -> `A & B`
- OR -> `A | B`
- no explicit combiner label tags (`[AND]`, `[OR]`, `[NONE]`).
- Apply frozen state tokens:
- `commandState`: `ON/OFF`
- `actualState`: `HIGH/LOW`
- `edgePulse`: `TRIG/CLR`
- `missionState`: `IDLE/RUN/DONE`
- constants with source card reference (`DO2:TRUE`, `AI1:FALSE`).
- Badge color reflects runtime evaluation true/false.

Done when:
- Set/reset badges remain single-line and readable on smartphone and desktop.
- Runtime evaluation color changes do not alter badge text format.
- Example expressions from documentation render exactly as specified.
- Live page makes no extra per-scan config API requests after initial bootstrap.

## 3.2 Card-By-Card Live Badge Rollout Plan

Execution policy:
- Implement and validate one family at a time.
- Keep runtime endpoints unchanged; use one-time config bootstrap cache for static text.
- Keep set/reset expression text stable; use runtime booleans only for badge active/inactive coloring.
- Enforce strict 4-zone card layout in all family slices:
- Zone 1 header, Zone 2 primary runtime, Zone 3 logic strip, Zone 4 debug actions.
- Zone 4 rendered only when `debugModeEnabled=true`.
- Use logical terminal identity labels (`DI0/DO1/...`) as primary visible card reference.

### Slice C1: DI First (Start Here)

Goal:
- Deliver complete DI live card visualization as baseline pattern for all later families.

Scope:
- Add DI card icon and DI-specific static parameter badges:
- `channel`, `invert`, `edgeMode`, `debounceMs`
- Apply strict 4-zone DI layout:
- Zone 1 uses `DIx` terminal identity and dominant state indicator.
- Zone 2 shows DI runtime row (`CMD`, `PHYS`, `EDGE`, `COUNT`).
- Zone 3 shows `SET`/`RST` expression strip.
- Zone 4 reserved for debug controls only and hidden when debug mode is disabled.
- Render DI set/reset expression badges using configured condition blocks:
- expression format `A`, `A & B`, `A | B`
- token vocabulary `ON/OFF`, `HIGH/LOW`, `TRIG/CLR`, `IDLE/RUN/DONE`, `TRUE/FALSE`
- Use runtime fields for dynamic DI badges:
- `commandState`, `actualState`, `edgePulse`, `liveValue`
- Use `turnOnConditionMet` and `turnOffConditionMet` only for SET/RST badge color state.

Done when:
- DI cards show all static parameter badges and set/reset expression badges at all times.
- DI set/reset badge color follows runtime evaluation without changing expression text.
- DI rendering is stable on mobile and desktop widths.
- Disabled DI cards keep Zones 2/3 visible in muted inactive style and show `DISABLED` in header.

### Slice C2: DO + SIO

Scope:
- Add DO/SIO static parameter badges.
- Add DO/SIO runtime mission-state badges and counters.
- Apply same set/reset expression renderer as DI.

### Slice C3: AI + Alarm(RTC)

Scope:
- Add static parameter badges for AI and Alarm.
- No set/reset badges for AI/Alarm; show explicit static `No SET/RST` badge.
- Render family-relevant runtime badges only.

### Slice C4: MATH

Scope:
- Add static MATH operation/input/range badges.
- Add MATH set/reset expression badges and runtime value badges.
- Final cross-family visual polish and compactness pass.

## 3.1 Card Wizard Follow-Up Slices (After P4)

## Slice P5: Live Card Wizard Shell + Debug Gating

Goal:
- Add guided per-card edit popup entry from Live cards with strict debug-mode gating.

Scope:
- Add per-card settings icon on Live cards.
- Show icon only when `debugModeEnabled=true`.
- Build wizard shell (stepper, back/next/cancel, loading/error patterns).
- Add read-only card summary and diff-preview placeholders.

Done when:
- Settings icon is not visible when debug mode is disabled.
- Clicking settings icon opens wizard for selected card in debug mode.
- Wizard can close/cancel without mutating device state.

## Slice P6: Live Card Wizard Full Card Edit + Save/Reboot Apply

Goal:
- Complete full per-card editing in wizard and apply through save + reboot flow.

Scope:
- Support full per-card parameter editing including set/reset condition rules.
- Submit through existing `GET/PUT /api/v3/cards/{id}` API path.
- Finish step shows explicit final confirmation and changed-fields summary.
- On successful save, trigger reboot flow and reconnect UX.

Done when:
- Operator can edit one card end-to-end from Live wizard and device reboots/reconnects automatically.
- Validation failures are mapped to wizard step/field feedback.
- Save failure and reboot failure paths are handled with clear operator messaging.

## 4. Minimal Weekly Cadence

- Monday: lock one slice scope and acceptance checks.
- Tuesday-Wednesday: implement only that slice.
- Thursday: hardware validation + bug fixes.
- Friday: update milestone/worklog and freeze.

## 5. Immediate Start (Today)

1. Start `P1` only.
2. Create a short test script for reconnect scenarios (manual steps acceptable).
3. Do not touch Config Studio this week.
