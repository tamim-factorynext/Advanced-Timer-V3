# Advanced Timer V3

Date: 2026-03-01
Status: Active Development (V3 production rewrite)

## 1. What This Project Is

Advanced Timer V3 is a deterministic control firmware platform for industrial-style logic execution on ESP32-class hardware, with a web portal for configuration, diagnostics, and commissioning.

This repository contains:
- Firmware runtime and logic engine.
- Portal/API transport layer.
- Contract documents for schema, API, timing, safety, and acceptance.
- Legacy V2 contract archive for historical traceability.

Target users of this document:
- Firmware contributors.
- Hardware contributors (current and future board revisions).
- Internal technical stakeholders (engineering leads, managers, QA, production).
- Technical users who need to understand behavior deeply before deployment.

Primary target users of the final off-the-shelf product:
- People currently using traditional multifunction timer modules who are afraid of PLC IDE complexity.
- Teams that need more capability than a timer module but do not want full PLC engineering overhead for everyday deployment and maintenance.

## 2. Current Program Direction

V3 is a clean production rewrite path, not incremental patching of V2 behavior. The deterministic runtime and the portal/networking services are intentionally separated by core ownership and clear module boundaries.

Canonical V3 contracts:
- `requirements-v3-contract.md`
- `docs/schema-v3.md`
- `docs/api-contract-v3.md`
- `docs/acceptance-matrix-v3.md`
- `docs/hardware-profile-v3.md`
- `docs/timing-budget-v3.md`
- `docs/fault-policy-v3.md`
- `docs/decisions.md`

Legacy archive:
- `docs/legacy/v2-poc-contract.md`

## 3. Product Goals and Non-Goals

### 3.1 Goals

- Deterministic, transparent logic evaluation.
- No-code structured configuration via LogicCards.
- Contract-driven validation before commit.
- Safe runtime operation with observability and rollback.
- Extensible hardware profile model across product variants.

### 3.2 Non-Goals

- No free-form scripting language.
- No hidden alternate execution path that bypasses deterministic rules.
- No mandatory dependency on one fixed hardware model.

### 3.3 Landscape and Positioning

The control/automation landscape already includes:
- Traditional PLC platforms (high-end and budget lines).
- OpenPLC and PLC-like open ecosystems.
- ESP32 PLC-style hardware projects that usually expect custom firmware development.
- Single/dual-channel timer products, including RTC timer variants.
- Home automation/control ecosystems (`Home Assistant`, `ESPHome`, `Tasmota`).
- Firmware development stacks (`Arduino`, `PlatformIO`, `ESP-IDF`, `MicroPython`).
- IoT cloud ecosystems (`Blynk`, `Particle`, `The Things Network`).

Advanced Timer V3 is positioned as:
- A deterministic, contract-driven controller firmware platform.
- A practical migration path for timer-module users who need more capability without entering full PLC IDE workflows.
- One tier above traditional multifunction timer modules in capability, observability, and lifecycle safety.
- More structured and production-oriented than DIY "write-your-own-firmware" ESP32 projects.
- More flexible and transparent than many fixed-function timer products.
- Complementary to, not a replacement for, cloud/home-automation ecosystems (it can integrate at system boundaries while keeping deterministic local control).

### 3.4 Distilled Market Research Essence

Based on external landscape research, this project addresses a specific gap between DIY firmware ecosystems and expensive/rigid industrial control stacks.

Key gap summary:
- Many ESP32 ecosystems are powerful but assume firmware/toolchain confidence from the user.
- Traditional timer modules are easy to use but limited in observability, lifecycle safety, and extensibility.
- Small OEM and installer workflows need local-first reliability, controlled updates, and practical protocol interoperability.

Strategic response in this project:
- Keep no-code, structured configuration with deterministic behavior.
- Add professional guardrails: staged validation/commit, rollback, runtime metrics, and clear ownership boundaries.
- Support field-friendly operation for users avoiding PLC IDEs, while preserving a deeper contract layer for contributors and advanced integrations.

Why this project exists:
- To provide a commercially credible bridge between basic timer products and full PLC programming ecosystems.
- To enable capable automation behavior without forcing every end user into PLC engineering workflows.

Open-source direction:
- Current focus is product/architecture hardening.
- If feasible, future release strategy may include open-sourcing part or all of the framework with clear contracts and contribution boundaries.

### 3.5 Competitive and Tooling Context (Short View)

This project is not being built in a vacuum. We acknowledge a broad existing ecosystem:
- ESP32 firmware ecosystems and tools: `ESPHome`, `Tasmota`, `Arduino`, `PlatformIO`, `ESP-IDF`, `MicroPython`, plus cloud/device ecosystems such as `Blynk`, `Particle`, and `The Things Network`.
- PLC/industrial ecosystems: traditional PLC platforms, IEC 61131-3 toolchains, HMI/SCADA stacks, and established OT integration patterns.
- Hardware-specific controller firmware projects and timer-class products already used in the field.

Named examples from market research (non-exhaustive):
- ESP32/open firmware and platforms: `ESPHome`, `Tasmota`, `Xedge32`, `Project Aura`.
- ESP32 hardware+firmware vendors/projects: `KinCony` (`KCS-V3`), `EQSP32`.
- Industrial/open control references: `OpenPLC`, `HomeMaster`, `BAScontrol`, `Siemens Desigo`.
- Silicon/platform ecosystem anchor: `Espressif` (ESP32 family evolution).

Our approach is simply a different fit point in that market:
- More capable and safer lifecycle than typical multifunction timer modules.
- Lower operational and learning burden than full PLC IDE-centric workflows for the target segment.
- Local-first deterministic behavior with practical web-based usability and integration paths.

## 4. System Architecture (Firmware + Portal)

### 4.1 Two-Core Ownership

Core ownership is strict:
- Core0 (`deterministic core`): logic card evaluation, scan loop, deterministic runtime state transitions, bounded-time command application.
- Core1 (`networking core`): web portal, WebSocket/API handling, WiFi policy, persistence workflows, non-deterministic and background services.

This split protects deterministic behavior from jitter introduced by network, UI, file I/O, and future service growth.

### 4.2 Design Decision: RTC Scheduling on Networking Core

RTC schedule matching is minute-level by contract and does not require per-scan deterministic execution.

Decision:
- RTC schedule-time checks run on the networking core.
- Evaluate once per minute and issue state-change intents to deterministic runtime via command/queue boundary.

Why:
- Prevents unnecessary load on deterministic scan loop.
- Scales better with large scheduler counts (for example 24 RTC scheduler cards).
- Keeps deterministic core focused on hard timing responsibilities.

### 4.3 Design Decision: Non-Deterministic Services Stay on Networking Core

All non-time-critical workloads are assigned to networking core, including future features:
- Data logging.
- Remote monitoring.
- Remote control interfaces.
- Periodic sensor reading that does not require scan-level determinism.
- Modbus RTU master behavior (if added) via adapter/service layer.

Rule:
- These services must interact with deterministic state through defined command/data contracts, not direct state mutation.

## 5. Firmware Domain Model

### 5.1 LogicCard Engine

LogicCards form a fixed ordered evaluation list. Families include:
- `DI` (Digital Input)
- `DO` (Digital Output)
- `AI` (Analog Input)
- `SIO` (Soft IO)
- `MATH`
- `RTC` (schedule/alarm semantics)

Each card follows shared set/reset condition structure and per-family validation rules defined in schema and requirements contracts.

### 5.2 Determinism Contract

Core runtime guarantees:
- Deterministic scan order.
- Deterministic run mode semantics (`RUN_NORMAL`, `RUN_STEP`, `RUN_BREAKPOINT`) with adjustable normal scan interval.
- Measured timing and queue metrics in runtime snapshot.
- No unbounded work in deterministic path.

### 5.3 Config Lifecycle and Rollback

Configuration lifecycle is staged:
- Load active.
- Save staged.
- Validate staged.
- Commit/deploy.
- Restore from LKG / slots / factory baseline.

This enables safe commissioning and controlled rollback.

## 6. Portal and API Model

Transport currently uses HTTP and WebSocket.

Typical endpoints include:
- `/api/snapshot`
- `/api/command`
- `/api/config/active`
- `/api/config/staged/save`
- `/api/config/staged/validate`
- `/api/config/commit`
- `/api/config/restore`

UI pages currently include:
- `/` (live/runtime)
- `/config` (configuration)
- `/settings` (system settings)

API payload contracts are defined in `docs/api-contract-v3.md`.

## 7. Build and Runtime Baseline

Platform baseline (`platformio.ini`):
- `platform = espressif32`
- `board = esp32doit-devkit-v1`
- `framework = arduino`
- `filesystem = littlefs`

Current library dependencies include:
- `ArduinoJson`
- `WebSockets`
- `RTClib`

Bring-up note:
- Current scan interval in implementation may be relaxed for validation bring-up.
- Production target remains 10 ms deterministic scan cadence, validated by timing contracts.

## 8. Testing Strategy (Functional + Performance + Production)

Testing is treated as a core product requirement, not a post-feature activity.

### 8.1 Why We Test

1. Functional validation: behavior matches contract and regressions are detected quickly.
2. Performance and stress validation: runtime remains stable and measurable under load.
3. Production verification: every manufactured unit is screened before shipment.

### 8.2 What Must Be Tested

Two major surfaces:
- Deterministic logic engine behavior.
- Portal/API behavior and configuration workflows.

Test groups:
- Card-family functional tests.
- Config lifecycle tests (save/validate/commit/restore).
- Command path tests (latency, queue depth, correctness).
- Long-run stability tests.
- Fault/degraded-mode tests.
- Security/authorization tests as features land.

### 8.3 Test Rig Recommendation: Raspberry Pi vs ESP32

Recommended primary controller: Raspberry Pi.

Reasoning:
- Better for orchestrating many concurrent test channels.
- Easier integration with CI, logs, database, dashboards, and production test station UX.
- Strong USB/serial/network tooling for automated test execution.

When an extra ESP32 is still useful:
- Electrical/protocol edge-case simulation close to target MCU behavior.
- Low-cost distributed signal generator/sensor emulator nodes.

Best practical approach:
- Raspberry Pi as test master.
- Optional one or more ESP32 helper nodes as deterministic I/O emulators.

### 8.4 Hardware-in-the-Loop (HIL) Rig Scope

Rig capabilities should include:
- Simulate DI and AI patterns.
- Observe/sense DO outputs.
- Timestamp event-response latency.
- Inject fault conditions (noise, disconnects, stale data).
- Run scripted regression sequences and save artifacts.

The same rig architecture should support:
- Engineering regression and stress runs.
- Production end-of-line pass/fail testing.

## 9. Hardware-Level Direction (Future)

Firmware is being structured for future hardware expansion:
- Profile-based family capacities (`0..N`) per build target.
- Adapter boundaries for local and remote I/O backends.
- Optional multi-board or split-MCU product models while preserving deterministic logic contract.

Hardware documentation can expand under `docs/` while keeping this README as the high-level system narrative.

## 10. Repository Map

- `src/main.cpp`: runtime bring-up and integration wiring.
- `src/kernel/`: deterministic card model and enums.
- `src/runtime/`: shared runtime snapshot model.
- `src/control/`: command DTO boundaries.
- `src/portal/`: route/portal boundary.
- `src/storage/`: config lifecycle storage interfaces.
- `data/`: portal static assets.
- `docs/`: active V3 contracts and engineering docs.
- `docs/legacy/`: historical artifacts and frozen V2 contract.

## 11. Contribution Workflow

Expected workflow for contributors:
1. Read this README to understand architecture and design intent.
2. Read the active contracts in `docs/` and `requirements-v3-contract.md`.
3. Make contract updates first (or together) for behavior/API/schema changes.
4. Implement code changes respecting layer and core boundaries.
5. Add/adjust tests and acceptance mappings.
6. Update `docs/decisions.md` for meaningful behavioral decisions.

Core engineering rules:
- Keep deterministic and non-deterministic concerns separated.
- Avoid direct cross-layer coupling.
- Prefer explicit contracts over implicit assumptions.

## 12. Immediate Next Engineering Milestones

- Finalize and implement RTC family in V3 runtime with minute-level matching policy on networking core.
- Define and implement non-deterministic service boundary for future data logging and remote features.
- Build first HIL testing rig (Pi-mastered) for automated functional and stress validation.
- Freeze production test profile and pass/fail criteria for manufacturing.

## 13. Canonical References

- V3 requirements: `requirements-v3-contract.md`
- V3 docs index: `docs/INDEX.md`
- V3 decisions: `docs/decisions.md`
- V2 frozen contract: `docs/legacy/v2-poc-contract.md`

## 14. Status Note

This README is the project-level long-form introduction and orientation document.

Detailed normative contracts remain in the V3 contract documents listed above. If there is any conflict, the contract files are authoritative for implementation and testing behavior.
