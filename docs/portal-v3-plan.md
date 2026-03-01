# V3 Portal Plan (Vision-Aligned)

Date: March 1, 2026
Status: Living Document (Execution Baseline)

## 1. Product Vision

The Advanced Timer V3 portal is a field-ready no-code control interface for professional deployments.

The intended experience is:
- Safe for non-programmers to operate without external training.
- Clear under stress (alarms, overrides, unstable connectivity).
- Deterministic and trustworthy: runtime truth always comes from firmware snapshots.

### 1.1 Product Position: Professional-Lite

"Professional-Lite" means:
- Easier than PLC IDE workflows.
- Safer and more explicit than hobby dashboards.
- Structured for installation, operations, and maintenance roles.

### 1.2 Self-Explanatory Mandate (Non-Negotiable)

The UI must explain itself in context. Users should not need separate tutorials to complete core tasks.

Required UX mechanisms:
- In-place help for every parameter and runtime control.
- First-run guided path for: connect -> inspect runtime -> edit staged config -> validate -> commit.
- Explicit state labeling that prevents confusion between staged and active configs.

### 1.3 Product-Led Non-Goals (Drift Guard)

The portal plan is anchored to your product vision, not market feature imitation.

Non-goals for V3 planning decisions:
- Do not add features only because competitors/products have them.
- Do not prioritize aesthetics over operational clarity and safety.
- Do not expand onboarding methods (for example BLE/Web Serial) unless they directly support your approved release scope.
- Do not combine technician maintenance workflows with day-to-day operator configuration in ways that increase misuse risk.

---

## 2. Vision Compatibility Gaps To Eliminate

This section captures why the previous plan felt only partially compatible with the vision.

1. The prior plan listed features but lacked measurable "self-explanatory" acceptance criteria.
2. It did not define explicit operator-safety UI invariants for high-risk actions.
3. Secondary features (maintenance panel, branding) were not prioritized against contract gates.
4. It did not map portal milestones to acceptance matrix IDs, making completion ambiguous.

---

## 3. Hard Constraints (From Contract + Decisions)

These are mandatory and override design preference:

- Fixed runtime status header always visible with run mode, override state, alarm, connectivity.
- Card render order must match firmware deterministic scan order.
- Portal must never recompute authoritative runtime logic values.
- Staged edits remain local until explicit save/validate/commit actions.
- Runtime controls must reflect kernel-acknowledged state, not optimistic state.
- Decimal <-> centiunit conversion is a portal responsibility.
- Role-based authorization (`VIEWER`, `OPERATOR`, `ENGINEER`, `ADMIN`) must gate protected actions.
- Frontend is rebuilt from fresh baseline per `DEC-0019` (do not patch legacy UI incrementally).

---

## 4. UX Invariants (Vision Contract)

The portal is considered vision-compatible only if all invariants below hold:

1. **Runtime truth is singular**
   - Every live state visible in UI is sourced from latest snapshot/event payload.
2. **Active vs Staged cannot be confused**
   - Persistent visual separation, labels, and action context for each state.
3. **Dangerous actions are explicit**
   - Force/mask/commit/restore always show current target, role requirement, and acknowledgement result.
4. **Mobile-first field usability**
   - Touch-safe controls, high-contrast status, and reduced typing burden.
5. **Role clarity by default**
   - Users see only controls appropriate to role; unavailable actions are explained, not silently hidden.

---

## 5. Information Architecture (Screens)

1. **Live Runtime**
   - Snapshot-driven card states, alarms, overrides, and metrics.
2. **Configuration Workspace**
   - Staged editor for card parameters and variable bindings.
3. **Validation & Commit**
   - Structured errors tied to fields; transactional commit status.
4. **Maintenance Panel**
   - Device health, network diagnostics, firmware management, recovery actions.
5. **Settings & Access**
   - User SSID config, role/session controls, theming/branding options.

---

## 6. Implementation Phases With Exit Gates

### Phase 1: Foundation

Build:
- `portal/` app scaffold (React + TypeScript + Vite).
- Bootstrap-based UI system, icon system, theme tokens.
- Typed HTTP/WebSocket client and shared DTO validation layer.
- Global app state with explicit separation: `runtime`, `stagedConfig`, `session`, `ui`.

Exit gate:
- Core app boots from firmware API mocks and real device endpoint.
- No runtime derivation logic outside API payload interpretation.

### Phase 2: Runtime + Config Lifecycle

Build:
- Fixed runtime header and live snapshot view.
- Active config load, staged edit flow, validate, commit UI.
- Clear staged/active visual model with irreversible ambiguity removed.

Exit gate (must pass):
- `AT-UI-002`, `AT-UI-003`, `AT-UI-004`, `AT-API-001`, `AT-API-002`, `AT-API-003`.

### Phase 3: Card Editors + Roles + Settings

Build:
- Editors for `DI`, `AI`, `SIO`, `DO`, `MATH`, `RTC`.
- Variable assignment (`CONSTANT`, `VARIABLE_REF`) UI with compatibility guards.
- Login/session and role-based action gating.
- WiFi User SSID settings.

Exit gate (must pass):
- `AT-DATA-001`, `AT-DATA-003`, `AT-BIND-001..006`, `AT-SEC-001..003`, `AT-WIFI-001..004`.

### Phase 4: Self-Explanatory + Maintenance + Polish

Build:
- Context help for all parameters and runtime commands.
- Guided onboarding flow for first-time users.
- Dedicated maintenance panel with operational diagnostics.
- Brand/theming controls constrained by readability and safety contrast rules.

Exit gate:
- First-run user can complete core commissioning without external docs.
- Mobile usability smoke and role-based workflows validated on hardware.

---

## 7. Traceability Matrix (Vision -> Contract -> Acceptance)

| Vision Objective | Contract/Decision Anchor | Acceptance Anchor |
| --- | --- | --- |
| Always-visible operational awareness | Sec 14.1 | `AT-UI-001` |
| Deterministic runtime trust | Sec 14.2 | `AT-UI-002`, `AT-UI-003` |
| No staged/active ambiguity | Sec 14.2 | `AT-UI-004` |
| Safe command acknowledgement | Sec 15.1, 15.2 | `AT-API-001`, `AT-API-005` |
| Numeric correctness at UI boundary | Sec 4.1, 4.2 | `AT-DATA-001`, `AT-DATA-003` |
| Role-safe operations | Sec 16 | `AT-SEC-001..003` |
| Fresh V3 UI baseline | `DEC-0019` | Delivery review checkpoint |

---

## 8. Out Of Scope For V3.0 (Explicit)

To maintain schedule and quality, the following are excluded from V3.0 unless contract changes:

- Remote/cloud multi-device fleet management.
- Full white-label template marketplace.
- BLE/Web Serial onboarding if firmware-side support is not stable in current release branch.

These can be planned for V3.1+ after V3.0 acceptance gates are green.

---

## 9. Definition Of Done For Portal V3

Portal V3 is done only when:

1. Contract-critical UI/API acceptance tests pass for portal scope.
2. Hardware smoke confirms stable operation under runtime and command load.
3. Self-explanatory onboarding and help coverage are complete for all card families.
4. Role and safety behaviors are auditable and predictable in failure paths.
