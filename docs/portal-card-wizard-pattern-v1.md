# Portal Card + Wizard Pattern v1

Date: 2026-03-11  
Status: Active implementation reference (DI completed baseline).  
Audience: Engineers extending Live Card and Wizard UX to AI/DO/SIO/MATH/RTC.

## 1. Purpose

This document captures the current DI implementation state across both:

- Live Card (runtime view on Live page)
- Card Wizard (guided setup/edit flow)

Use this as the replication blueprint for other card families.

## 2. Current DI Live Card State (Implemented)

DI live card is implemented in `data/index.html` (`renderDiCard`) and follows these contracts.

### 2.1 Live Card Structure

- Header: family title + logical index + dominant state token.
- Zone 1: static config badges (`SIG`, `TRG`, `DEB`).
- Zone 2: runtime output badges (`CMD`, `PHYS`, `EDGE`, `COUNT`).
- Zone 3: logic expression badges (`SET`, `RST`) with evaluation color.
- Zone 4 (debug only): force controls (`LOW/REAL/HIGH`) + `Edit` entry button.

### 2.2 Live Card Behavior Rules

- Disabled card keeps context visible but muted.
- `SET`/`RST` badge color is evaluation-driven when enabled.
- Debug actions are shown only when `debugModeEnabled=true`.
- Force mode selection updates runtime command state through existing command endpoint.

### 2.3 Live Card Terminology Contract

- `CMD`: qualified logical state.
- `PHYS`: effective physical state.
- `EDGE`: one-scan trigger pulse.
- `COUNT`: qualified edge counter.

## 3. Current DI Wizard State (Implemented)

DI wizard is implemented in `data/index.html` and follows these principles:

- Single-mode flow: configuration + guided learning in one experience.
- Setup-only guidance: no live runtime stream inside wizard pages.
- Low-segmentation visual style: reduced nested container chrome.
- Inline SVG concept visuals for teaching behavior.
- Deterministic helper copy per step:
  - `What this field does`
  - `With this config, ...`

### 3.0 Onboarding Pattern Decision (Locked)

- Keep two intro steps at the start of the wizard as the default pattern.
- `Intro A` is for core behavior model (signal path, gating, reset-dominance).
- `Intro B` is for output literacy (`CMD/PHYS/EDGE/COUNT`) and cross-card usage.
- Rationale: this improves first-time user intuitiveness before field-level editing.

### 3.1 Step Model (8 Steps)

1. `Intro A`: DI flow + gating/reset basics + enable state.
2. `Intro B`: DI outputs (`CMD/PHYS/EDGE/COUNT`) and usage guidance.
3. `Signal`: `invert` behavior.
4. `Edge`: edge qualification mode.
5. `Debounce`: stability window.
6. `SET`: turn-on condition authoring.
7. `RST`: turn-off/reset condition authoring.
8. `Review`: deterministic summary + reboot reminder.

### 3.2 Wizard UX Contracts

- `reset` dominance remains explicitly visible on SET/RST pages.
- Save semantics unchanged:
  - Save applies config payload.
  - Effective runtime behavior changes after reboot.
- Stepper, Back/Next, Save & Reboot flow remains stable.
- Wizard panel is viewport-capped and body-scrollable.

## 4. Replication Blueprint (Other Families)

Use DI as the canonical template and split work into Live Card + Wizard.

### 4.1 Keep (Shared Infrastructure)

- Same modal shell and stepper behavior for all family wizards.
- Same compact visual language and spacing rules.
- Same deterministic copy slots per wizard step.
- Same two-intro-step onboarding pattern (`Intro A` + `Intro B`) before edit steps.
- Same live-card zone ordering pattern where family behavior allows it.
- Same debug gating model for actions.

### 4.2 Replace (Family-Specific Content)

- Live card runtime badges and expression grammar.
- Pipeline labels and active-node mapping.
- Concept visuals and instructional copy.
- Condition/control composer sections (if family uses set/reset).
- Review profile fields tied to family semantics.

## 5. Implementation Checklist (Per New Family)

- Live Card:
  - define header token + runtime badges + logic badges + debug zone behavior
  - enforce muted disabled state behavior
  - map tokens to canonical naming
- Wizard:
  - define step metadata + flow
  - implement step field sync
  - add concept visuals + deterministic outcome copy
  - implement review profile summary + reboot note
- Integration:
  - ensure save path uses existing API contract
  - ensure live card reflects saved config after reboot

## 6. Acceptance Criteria (Per New Family)

- Live Card:
  - operators can read dominant state and key outputs quickly
  - debug controls appear only when allowed
- Wizard:
  - usable at 100% desktop zoom and mobile tall view
  - no clipping of critical actions
  - user can complete setup without leaving wizard
- Regression:
  - no DI behavior regression after shared refactors

## 7. Regression Guardrails

- Do not reintroduce heavy visual fragmentation (nested card chrome).
- Do not introduce runtime-live dependency inside wizard guidance.
- Do not change save/reboot semantics in wizard without explicit contract update.
- Keep naming aligned across Live Card, Wizard, and user docs.
