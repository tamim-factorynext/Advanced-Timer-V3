# Portal Card + Wizard Pattern v1

Date: 2026-03-12  
Status: Active implementation reference (DI/AI/DO/SIO/MATH/ALARM live + wizard implemented).  
Audience: Engineers extending Live Card and Wizard UX to AI/DO/SIO/MATH/RTC.

## 1. Purpose

This document captures the current family implementation state across both:

- Live Card (runtime view on Live page)
- Card Wizard (guided setup/edit flow)

Use this as the maintenance and refinement baseline.

## 2. Live Card State (Implemented Families)

Live cards are implemented in `data/index.html` with family renderers:

- `renderDiCard`
- `renderAiCard`
- `renderDoCard`
- `renderSioCard`
- `renderMathCard`
- `renderAlarmCard`

### 2.1 Live Card Structure

- Header: family title + logical index + dominant state token.
- Zone 1: static config badges (family specific).
- Zone 2: runtime output badges (family specific).
- Zone 3: logic expression badges (`SET`, `RST`) where supported.
- Zone 4 (debug only): family action row + `Edit` entry button.

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

## 3. Wizard State (Implemented Families)

Wizards are implemented in `data/index.html` and follow these principles:

- Single-mode flow: configuration + guided learning in one experience.
- Setup-only guidance: no live runtime stream inside wizard pages.
- Low-segmentation visual style: reduced nested container chrome.
- Humanized one-line guidance near the active control.
- Reduced segmentation and tighter vertical coupling for related controls.

### 3.0 Onboarding Pattern Decision (Locked)

- Keep two intro steps at the start of the wizard as the default pattern.
- `Intro A` is for core behavior model (signal path, gating, reset-dominance).
- `Intro B` is for output literacy (`CMD/PHYS/EDGE/COUNT`) and cross-card usage.
- Rationale: this improves first-time user intuitiveness before field-level editing.

### 3.1 Step Models

- DI: 8 steps
- AI: 7 steps
- DO/SIO: 10 steps
- MATH: 12 steps
- Alarm (RTC): 4 steps

### 3.2 Wizard UX Contracts

- `reset` dominance remains explicitly visible on SET/RST pages.
- Save semantics unchanged:
  - Save applies config payload.
  - Effective runtime behavior changes after reboot.
- Stepper, Back/Next, Save & Reboot flow remains stable.
- Wizard panel is viewport-capped and body-scrollable.

## 4. Implementation Notes (Current)

- Step text row is hidden in UI; stepper pills remain visible.
- Header hierarchy uses de-emphasized card title + emphasized instructional subtitle.
- Guidance is rendered immediately below key action/edit control on intro pages.
- Dashed separators in guidance blocks are removed in single-action layouts.
- Save semantics are unchanged: save writes config, reboot activates behavior.

## 5. Next Refinement Checklist

- Tune per-family text density for DO/SIO/MATH/Alarm.
- Validate mobile spacing for long subtitle lines.
- Add consistency pass for remaining icon/label language.
- Re-check all live card badges against runtime data in field tests.

## 6. Acceptance Criteria (Refinement)

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
