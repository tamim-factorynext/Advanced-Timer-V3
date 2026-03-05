# Core Contract (V4)

Date: 2026-03-05
Status: Draft (active)
Supersedes for active work: V3 runtime-core + Wi-Fi sections

## 1. Purpose

Define the V4 runtime core contract for ESP32-S3, with focus on:

- dual-core ownership and scheduling boundaries
- Wi-Fi connectivity behavior and constraints

This contract imports V3 baseline rules and applies V4-specific adjustments.

## 2. Imported Baseline (From V3)

Imported from `legacy/requirements-legacy-v3-contract.md`:

- Core ownership model:
  - Core0 owns deterministic kernel state and scan execution.
  - Core1 owns network/portal/storage/control transport.
  - Cross-core communication must use bounded channels.
- Wi-Fi constraints:
  - STA mode only.
  - Non-blocking networking behavior must not interfere with Core0 timing.
- Connectivity fallback:
  - Dual-SSID strategy (backup + user-configured).
  - Boot-time connection sequence with offline-mode fallback.

## 3. V4 Core Runtime Contract

### 3.1 Dual-Core Ownership (Mandatory)

- Core0 MUST run deterministic scan/runtime logic only.
- Core1 MUST run non-deterministic services (Wi-Fi, HTTP/WebSocket, config I/O, diagnostics transport).
- Kernel logic on Core0 MUST NOT call Wi-Fi/HTTP/filesystem APIs directly.
- Core1 -> Core0 commands MUST be queued through bounded, non-blocking interfaces.
- Core0 -> Core1 snapshots/events MUST be published through bounded queues with drop/overwrite policy defined in code.

### 3.2 Scheduling and Blocking Rules

- Core0 loop MUST be periodic and bounded by configured scan interval.
- Core0 MUST avoid unbounded waits, socket calls, and filesystem operations.
- Core1 MAY perform variable-latency operations, but MUST use timeouts and retry backoff.
- Any cross-core lock that can block Core0 is forbidden.

### 3.3 Wi-Fi Connectivity Contract (V4)

- Device MUST operate in `WIFI_STA` mode for production runtime.
- Device MUST support two credentials:
  - `backupAccessNetwork` (factory recovery path, non-editable by normal user flow)
  - `userConfiguredNetwork` (primary runtime network)
- Boot connect order MUST be:
  1. backup network (short timeout)
  2. user network (longer timeout)
  3. offline mode with periodic background retries
- Offline mode MUST keep deterministic runtime fully functional.
- Reconnect attempts MUST be non-blocking to Core0 and rate-limited.

### 3.4 Observability Hooks (Required by DEC-0001)

- Runtime MUST expose at least:
  - per-core CPU load estimate
  - heap/SRAM headroom
  - PSRAM usage/headroom (when present)
  - per-task stack high-water marks
  - flash and LittleFS usage
- Metrics collection MUST be low-overhead and interval-based.
- Metrics publishing MUST NOT violate Core0 timing guarantees.

## 4. V4 Modifications vs V3

- V3 clause "Core1 owns portal/network/filesystem/control transport" is kept, but V4 makes queue/backpressure behavior explicit as a required implementation contract.
- V3 Wi-Fi dual-SSID strategy is kept, but V4 formalizes retry rate-limiting and offline-first continuity as explicit mandatory behavior.
- V4 adds mandatory health telemetry hooks as part of core contract (from `docs/decisions.md`, DEC-0001).

## 5. Alignment Check with V4 Goals

Goal: deterministic ESP32-S3 rebuild with strong observability and resilient connectivity.

- Determinism: aligned
  - strict Core0 isolation from variable-latency subsystems
- Connectivity resilience: aligned
  - retained dual-SSID + offline fallback + retry policy
- Telemetry-first workflow: aligned
  - observability hooks included as contract requirements
- Migration practicality: aligned
  - reuses proven V3 rules, narrows and clarifies for V4 implementation

Verdict: This contract aligns with current V4 goals and can be used as the active implementation baseline for core scheduling and Wi-Fi runtime behavior.
