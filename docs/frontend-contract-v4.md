# Frontend Contract (V4)

Date: 2026-03-05
Status: Draft (active)
Source baseline: legacy portal plan + API contract + legacy HTML implementation

## 1. Purpose

This document defines V4 frontend behavior contracts for:

- portal UX and page responsibilities
- runtime data rendering and transport behavior
- config lifecycle UX and backend interaction model
- theming, iconography, stack usage, and performance budgets
- reliability, error handling, and operator safety patterns

When this contract conflicts with legacy V3 frontend behavior, this V4 contract is authoritative.

## 2. Product UX Positioning

The V4 portal is a professional-lite field UI:

- easier than full PLC IDE workflows
- more structured and auditable than hobby dashboards
- safe under operational stress (alarms, degraded network, forced/masked states)

Non-negotiables:

- runtime truth comes from firmware snapshots only
- staged/active confusion must be prevented by design
- dangerous actions require explicit acknowledgement and outcome visibility

## 3. Information Architecture and Page Map

### 3.1 Mandatory Page Set

V4 portal must ship with these pages:

- `Live` (`/`): runtime status, snapshot cards, runtime KPIs, operational context
- `Config` (`/config`): card configuration lifecycle, import/export/apply
- `Settings` (`/settings`): system/device settings, factory reset, recovery visibility
- `Learn` (`/learn`): onboarding/tutorial/examples

### 3.2 Navigation and Shell Rules

- top-level nav must be present and consistent on all pages
- active page state must be visually explicit
- header shell must preserve runtime connectivity context across navigation
- mobile safe-area behavior must keep key actions visible

## 4. Visual System Contract

### 4.1 Theme System

Theme system is mandatory and token-driven.

- frontend must use shared design tokens (CSS variables) for colors/surfaces/state indicators
- minimum preset support:
  - light
  - dark
  - solarized-style
  - two colorful variants
- theme choice must persist across pages/reload
- compatibility aliases from legacy may be supported:
  - `colorful-a -> neon-orchid`
  - `colorful-b -> velvet-sand`

Semantic color stability:

- status colors for critical operational states (`OK`, `WARN`, `FAULT`, forced/masked) must remain semantically stable across themes

### 4.2 Typography and Layout

- mobile-first sizing/tap targets are required
- readability-first typography is required for field conditions
- fixed runtime-awareness header/status zone is required

## 5. Iconography and Labeling Contract

- icon system must be consistent across pages/actions
- critical actions must include icon + text (no icon-only destructive controls)
- status icon mapping must be deterministic:
  - connected/healthy
  - warning/degraded
  - fault/error
  - forced/masked/test override

## 6. Frontend Stack and Dependency Contract

### 6.1 Stack Policy

Adopt frontend dependencies incrementally, validating each addition.

Recommended baseline order (from legacy plan, still valid for V4):

1. Bootstrap 5
2. Bootstrap Icons
3. Alpine.js
4. HTMX
5. Chart.js (optional, justified use only)

### 6.2 Usage Rules

- framework usage is allowed but V4 visual identity must not be default-framework-looking
- keep dependency footprint controlled; avoid heavy unused bundles
- pin versions for release builds
- prefer local-hosted assets in production builds (LittleFS) after validation

## 7. Runtime Data Contract (Frontend View)

### 7.1 Authority Rules

- snapshot and diagnostics payloads are authoritative
- frontend must not recompute logical runtime outcomes
- card display order must follow deterministic firmware order

### 7.2 Snapshot Rendering Rules

Runtime views must support and display:

- connection state
- scan metrics (`scanLastUs`, `scanMaxUs`, budget, overrun counters)
- queue metrics (depth/high-water/drop)
- card runtime fields (`commandState`, `actualState`, `edgePulse`, `liveValue`, family metadata)

### 7.3 Revision Rules

- WebSocket/HTTP snapshot revision ordering must be preserved
- stale data conditions must be visible to user
- unknown response fields must be tolerated for forward compatibility

## 8. Config Lifecycle UX Contract (V4)

### 8.1 One-Click Apply (Locked)

User interaction model is one-click apply.

- do not expose separate UI buttons for `save staged`, `validate`, `commit`
- expose one primary `Apply` action in Config page

### 8.2 Internal Transaction Pipeline

Backend must still execute transactional stages internally:

1. stage candidate
2. validate candidate
3. commit candidate

If any stage fails, activation must abort and frontend must show structured errors.

### 8.3 Import/Export Model

- export package must represent split V4 artifacts (`settings` + `card_config`) with compatible metadata
- import must run compatibility and validation preview before apply confirmation
- field-level and summary-level validation feedback must be shown

### 8.4 Restore Policy

- user restore options:
  - validated import apply (Config page)
  - factory reset (Settings page)
- user-facing restore-to-LKG is forbidden
- automatic LKG recovery remains internal and diagnostics-visible only

## 9. Page-Specific Behavior Contract

### 9.1 Live Page

- must show connectivity state (`CONNECTED|DEGRADED|OFFLINE|RECONNECTING`)
- must show loading/empty/error states explicitly
- must expose key operational chips/kpis and per-card state blocks

### 9.2 Config Page

- must support card browsing/editing by family and deterministic ordering
- must support condition-block editing with fixed payload shape requirements
- must support import/export/apply flow with clear action progress and result states
- staged-vs-active context must remain explicit

### 9.3 Settings Page

- owns system settings (network/time/preferences) and factory reset
- must expose recovery/system status visibility
- must not host card config lifecycle controls

### 9.4 Learn Page

- must provide three-track learning structure:
  - introduction/getting-started
  - tutorial walkthrough
  - curated examples
- content should support first-use commissioning without external docs

## 10. Transport Contract (Frontend Perspective)

- runtime stream: WebSocket + latest-snapshot HTTP read path
- config/settings actions: HTTP JSON endpoints (or equivalent RPC) with consistent metadata
- requests must follow single-flight policy by default
- retry/backoff must avoid overlap storms and silent retries

Minimum mutating-response metadata:

- `requestId`
- `timestamp`
- `status`
- `errorCode` (on failure)
- `snapshotRevision` where relevant

## 11. Error, Safety, and Action Acknowledgement

- every critical action must have explicit lifecycle state:
  - submitted
  - acknowledged/rejected
  - completion result
- validation errors must include machine-readable code and operator-readable message
- optimistic UI state is forbidden for safety-critical controls (force/mask/reset/apply)
- reconnect and offline states must not hide action failures

## 12. Telemetry and Reliability Signals

Frontend must emit structured telemetry events for:

- app lifecycle and route open
- transport connect/disconnect/reconnect attempts
- config apply/import/factory-reset action lifecycle
- validation/commit failures

Operational diagnostics should support incident replay and stale-data analysis.

## 13. Performance and Budget Guidance

- runtime updates should remain smooth under target polling/stream cadence
- frontend request model must remain single-flight
- compressed portal asset budget should remain controlled for embedded serving constraints
- heavy visualization dependencies require explicit value proof before adoption

## 14. V4 Deviations from Legacy V3

V4 intentionally changes these legacy behaviors:

- removes separate user buttons for save/validate/commit and replaces with one-click apply UX
- keeps transactional backend safety internally
- moves backup/restore lifecycle ownership to Config page
- limits Settings page to system-level controls (including factory reset and recovery visibility)
- keeps LKG internal-only, not user-restorable

## 15. Alignment Verdict

This frontend contract is aligned with V4 goals:

- deterministic-runtime authority preserved
- simpler operator flow through one-click apply
- safer restore policy with internal-only LKG
- practical embedded UI constraints retained
