# Advanced Timer V4 (ESP32-S3 Rebuild)

Status: Rebuild branch active (`esp32-s3-v4-rebuild`)

This branch is the clean-slate V4 implementation track focused on ESP32-S3.

## Project Identity

Advanced Timer V4 is a deterministic, configurable industrial control firmware in the timer-relay product class, implemented on ESP32-S3 (N16R8 target profile).

It is intentionally positioned between:

- simple multi-function timer relays (easy, limited)
- full PLC + IEC IDE stacks (powerful, heavy)

V4 uses strict contracts, bounded behavior, and a constrained feature model to deliver predictable control behavior without full PLC complexity.

Identity summary:

- deterministic local runtime first
- structured no-code configuration model (card-based)
- industrial-style lifecycle safety (validate/commit/rollback mindset)
- PLC-inspired control behavior without full PLC IDE burden

## Core Intent

Build a reliable control platform that offers PLC-inspired runtime patterns (cards, scan execution, stateful logic, scheduling) while remaining:

- constrained by design
- easier to deploy
- easier to validate
- safer to operate in fixed-use industrial contexts

Core behavior promise:

- no hidden execution path bypassing deterministic rules
- network/portal services must never become a dependency for deterministic control execution
- constrained feature set by design to preserve operational predictability

## Product Goals and Non-Goals

Goals:

- deterministic, transparent logic evaluation
- contract-driven validation before applying config changes
- safe field operation with explicit observability signals
- profile-gated scalability across hardware variants without changing logic semantics

Non-goals:

- free-form general-purpose scripting as the primary runtime model
- unrestricted feature sprawl that weakens determinism/safety
- mandatory dependence on a single fixed hardware model
- replacing full PLC engineering suites for high-complexity IEC-centric projects

## For Whom

Primary users:

- panel builders and machine integrators needing deterministic control with limited scope
- technicians/operators needing clear runtime visibility and safe control paths
- teams that need more than timer relays but less than full PLC software lifecycle overhead

Secondary stakeholders:

- firmware engineers needing explicit runtime contracts
- QA and validation teams requiring testable acceptance behavior
- product teams targeting commercially deployable edge-control devices

## Why This Exists

Common gap in the market:

- timer relays are simple but too rigid for evolving logic
- small PLC stacks can be overkill in cost, tooling, and maintenance burden
- many low-cost controllers lack determinism discipline, observability, or robust config lifecycle

V4 targets this gap with:

- deterministic scan-loop architecture
- explicit resource and telemetry tracking
- contract-first behavior definition
- profile-gated feature scope to control complexity

Operational framing:

- local-first control continuity is mandatory
- connectivity enhances operation but must not own runtime safety
- staged config lifecycle is preferred over ad-hoc direct mutation

## Market and Competition Snapshot

V4 is designed for practical competition against three groups:

1. Multi-function timer relays:
   - Strength: low cost, simple setup
   - Weakness: limited logic composition and poor observability
   - V4 response: richer logic model with deterministic behavior and diagnostics
2. Low-cost smart relay/micro-PLC devices:
   - Strength: moderate feature set
   - Weakness: mixed runtime transparency, inconsistent architectural discipline
   - V4 response: explicit contracts, telemetry-first operation, strict runtime boundaries
3. Full PLC + IEC IDE ecosystems:
   - Strength: deep capability and standard tooling
   - Weakness: higher complexity/cost for small to mid-scope use cases
- V4 response: focused constrained feature set for faster deployment and lower operational overhead

Ecosystem context (fit-point perspective):

- DIY/open firmware ecosystems (for example ESPHome/Tasmota class): flexible, but often require stronger user/tooling confidence than target field users have.
- Hardware-specific controller firmware ecosystems: may reduce setup friction but can carry portability, transparency, or lifecycle constraints.
- Traditional PLC/IEC ecosystems: proven and deep, but often heavier than needed for this product's target scope.

## Product Positioning Principles

- Be capable, but intentionally bounded.
- Prefer deterministic and auditable behavior over feature breadth.
- Optimize for stable field operation, not generalized programming-platform scope.
- Keep system understandable for timer-relay-class migration users while preserving professional guardrails.
- Treat observability, recovery behavior, and validation discipline as core product features, not optional extras.

## Source Of Truth (Current)

- Migration/rebuild execution plan: `docs/esp32-s3-psram-migration-guide.md`
- V4 contract anchor (draft): `requirements-v4-contract.md`

## Legacy Reference

- Previous root overview preserved as: `legacy/README-legacy-v3.md`
- Existing V3 contracts/docs are retained as reference until replaced by V4 equivalents.

## Immediate Next

1. Define V4 contracts for platform, storage, runtime, and transport.
2. Stand up minimal ESP32-S3 boot skeleton.
3. Add telemetry baseline (CPU/core load, SRAM, PSRAM, heap, stack high-water).

