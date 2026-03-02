# V3 Milestones

Date: 2026-03-02  
Status: Active checkpoint tracker for V3 execution progress.

## 1. Usage Rules

- Update this file when a milestone starts, completes, is blocked, or changes scope.
- Keep milestone records concise and evidence-driven.
- Link each milestone to decision IDs and key files.
- Record the commit short SHA for the checkpoint update.

Status vocabulary:

- `PLANNED`
- `IN_PROGRESS`
- `DONE`
- `BLOCKED`

## 2. Milestone Ledger

## M0: Branch Safety Baseline
- Status: `DONE`
- Date: 2026-03-02
- Summary: froze rollback-safe baseline before fast-track rewrite.
- Outputs:
  - branch `legacy-stable`
  - branch `v3-core`
  - tag `v2-legacy-baseline-2026-03-02`
- References:
  - `DEC-0020`
- Checkpoint SHA: `d3b1840`

## M1: Composition-Root Architecture Baseline
- Status: `DONE`
- Date: 2026-03-02
- Summary: replaced monolithic entrypoint with service-boundary composition root.
- Outputs:
  - `src/main.cpp`
  - `src/kernel/kernel_service.*`
  - `src/runtime/runtime_service.*`
  - `src/control/control_service.*`
  - `src/storage/storage_service.*`
  - `src/portal/portal_service.*`
  - `src/platform/platform_service.*`
- References:
  - `DEC-0021`
  - `requirements-v3-contract.md` (architecture boundaries)
- Checkpoint SHA: `d3b1840`

## M2: Typed Config Contract + Validation Boot Gate
- Status: `DONE`
- Date: 2026-03-02
- Summary: added typed config contract, validator, and kernel boot through validated config only.
- Outputs:
  - `src/storage/v3_config_contract.*`
  - `src/storage/v3_config_validator.*`
  - `src/storage/storage_service.*`
  - `src/kernel/kernel_service.*`
  - `src/main.cpp` (fail-fast on invalid config)
- References:
  - `DEC-0021`
  - `requirements-v3-contract.md` (validation and ownership rules)
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
- Checkpoint SHA: `d3b1840`

## M3: JSON Decode -> Typed SystemConfig
- Status: `DONE`
- Date: 2026-03-02
- Summary: parse persisted JSON payload into `SystemConfig` and route through validator at bootstrap.
- Outputs:
  - `src/storage/v3_config_decoder.*`
  - `src/storage/storage_service.cpp` bootstrap path:
    - LittleFS load (`/config_v3.json`) when present
    - JSON deserialize
    - decode to `SystemConfig`
    - validate to `ValidatedConfig`
  - explicit payload error mapping:
    - `config_payload_invalid_json`
    - `config_payload_invalid_shape`
    - `config_payload_unknown_family`
- References:
  - `docs/schema-v3.md`
  - `docs/api-contract-v3.md`
  - `src/storage/v3_config_validator.*`
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - memory snapshot: RAM `30792 / 327680` (9.4%), Flash `331021 / 1310720` (25.3%)
- Checkpoint SHA: `d3b1840`

## M4: Kernel Runtime Binding From Typed Config
- Status: `DONE`
- Date: 2026-03-02
- Summary: bind typed config into kernel-owned runtime summary at startup and expose counts through runtime snapshot.
- Outputs:
  - `src/kernel/kernel_service.cpp`:
    - bind per-family and enabled-card counts from validated `SystemConfig`
  - `src/kernel/kernel_service.h`:
    - extended kernel metrics for configured/enabled and family counts
  - `src/runtime/runtime_service.*`:
    - snapshot now mirrors kernel binding summary counts
- References:
  - `DEC-0021`
  - `DEC-0022`
  - `requirements-v3-contract.md` (state ownership and startup contract)
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:12.591`
  - RAM: `30808 / 327680` (9.4%)
  - Flash: `331209 / 1310720` (25.3%)
- Checkpoint SHA: `d3b1840`

## M5: Startup Invariant Checks For Config Binding Summary
- Status: `DONE`
- Date: 2026-03-02
- Summary: add kernel-owned consistency checks for typed config binding and expose result in runtime snapshot.
- Implemented outputs:
  - `src/kernel/kernel_service.*`:
    - `familyCountSum`
    - `bindingConsistent`
  - `src/runtime/runtime_service.*`:
    - runtime snapshot fields for binding invariants
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:13.476`
  - RAM: `30816 / 327680` (9.4%)
  - Flash: `331277 / 1310720` (25.3%)
- Checkpoint SHA: `d3b1840`

## M6: Storage-Kernel Bootstrap Diagnostics Surface
- Status: `DONE`
- Date: 2026-03-02
- Summary: expose bootstrap source/error diagnostics in runtime snapshot to speed field troubleshooting.
- Implemented outputs:
  - storage bootstrap diagnostics:
    - `BootstrapSource` (`DefaultConfig` vs `FileConfig`)
    - `BootstrapDiagnostics` (`source`, `error`, `hasActiveConfig`)
    - files: `src/storage/storage_service.h`, `src/storage/storage_service.cpp`
  - runtime snapshot wiring:
    - `bootstrapUsedFileConfig`
    - `storageHasActiveConfig`
    - `storageBootstrapError`
    - files: `src/runtime/runtime_service.h`, `src/runtime/runtime_service.cpp`
  - loop wiring:
    - `src/main.cpp` passes `gStorage.diagnostics()` into runtime tick
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:15.534`
  - RAM: `30824 / 327680` (9.4%)
  - Flash: `331397 / 1310720` (25.3%)
- Checkpoint SHA: `d3b1840`

## M7: Portal Diagnostics Surface (Bootstrap + Binding)
- Status: `DONE`
- Date: 2026-03-02
- Summary: expose new runtime diagnostics in portal-facing payload/view for quick commissioning checks.
- Implemented outputs:
  - portal diagnostics contract:
    - `PortalDiagnosticsState` (`ready`, `revision`, `json`)
    - `PortalService::diagnosticsState()`
    - files: `src/portal/portal_service.h`, `src/portal/portal_service.cpp`
  - runtime -> portal diagnostics serialization payload:
    - `binding` section:
      - configured/enabled/family sum/consistency
      - per-family counts
    - `bootstrap` section:
      - `usedFileConfig`
      - `hasActiveConfig`
      - `errorCode` (string from storage error mapping)
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:17.234`
  - RAM: `31344 / 327680` (9.6%)
  - Flash: `335361 / 1310720` (25.6%)
- Checkpoint SHA: `d3b1840`

## M8: Dual-Core Runtime Skeleton (Core0 Kernel / Core1 Services)
- Status: `DONE`
- Date: 2026-03-02
- Summary: move from single-loop execution to dual-core task scaffold with explicit core ownership split.
- Implemented outputs:
  - `src/main.cpp`:
    - Core0 task (`v3-core0-kernel`): kernel tick + shared metrics publish
    - Core1 task (`v3-core1-services`): control/runtime/portal ticks
    - protected metrics handoff via critical section (`portMUX_TYPE`)
    - setup initializes services, then starts both pinned tasks
    - `loop()` changed to idle task delay
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:12.793`
  - RAM: `31376 / 327680` (9.6%)
  - Flash: `335717 / 1310720` (25.6%)
- Checkpoint SHA: `d3b1840`

## M9: Queue-Based Core Boundary (Commands/Snapshots Skeleton)
- Status: `DONE`
- Date: 2026-03-02
- Summary: replace shared-struct cross-core handoff with bounded queue channels for deterministic ownership boundaries.
- Implemented outputs:
  - bounded queue handoff Core0 -> Core1:
    - `gKernelMetricsQueue` (`capacity=8`)
    - producer: `enqueueKernelMetrics(...)` on Core0
    - consumer: `latestKernelMetricsFromQueue()` on Core1
  - queue health counters:
    - `gKernelMetricsQueueDepth`
    - `gKernelMetricsQueueHighWater`
    - `gKernelMetricsQueueDropCount`
  - removed direct shared metrics critical-section copy path
  - file: `src/main.cpp`
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:15.408`
  - RAM: `31392 / 327680` (9.6%)
  - Flash: `335945 / 1310720` (25.6%)
- Checkpoint SHA: `d3b1840`

## M10: Command Queue Skeleton (Core1 -> Core0)
- Status: `DONE`
- Date: 2026-03-02
- Summary: add bounded command channel so Core1 service side can issue kernel commands via queue instead of direct ownership paths.
- Implemented outputs:
  - command channel in `src/main.cpp`:
    - `KernelCommandType`
    - `KernelCommand`
    - `gKernelCommandQueue` (`capacity=16`)
  - Core1 enqueue path:
    - periodic no-op heartbeat command (`250ms`) with sequence + enqueue timestamp
  - Core0 dequeue/apply path:
    - `applyKernelCommands(...)` drains queue and applies commands
  - command queue counters:
    - depth/high-water/drop
    - applied count/no-op count
    - last/max enqueue-to-apply latency (ms)
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:12.471`
  - RAM: `31416 / 327680` (9.6%)
  - Flash: `336253 / 1310720` (25.7%)
- Checkpoint SHA: `d3b1840`

## M11: Snapshot Queue Skeleton (Core0 -> Core1 Payload Path)
- Status: `DONE`
- Date: 2026-03-02
- Summary: add explicit snapshot payload queue boundary for Core0-produced runtime state consumed by Core1 services.
- Implemented outputs:
  - bounded snapshot message queue in `src/main.cpp`:
    - `KernelSnapshotMessage { producedAtMs, metrics }`
    - `gKernelSnapshotQueue` (`capacity=8`)
  - Core0 producer flow:
    - produce snapshot each kernel loop after tick
    - non-blocking enqueue with drop counter
  - Core1 consumer flow:
    - drain queue to latest snapshot
    - use snapshot payload for runtime tick
  - snapshot queue telemetry:
    - depth/high-water/drop counters
  - removed metrics-only queue path in favor of snapshot payload queue
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:12.293`
  - RAM: `31416 / 327680` (9.6%)
  - Flash: `336325 / 1310720` (25.7%)
- Checkpoint SHA: `d3b1840`

## M12: Runtime Queue Telemetry Surfacing
- Status: `PLANNED`
- Date: 2026-03-02
- Summary: expose command/snapshot queue health counters in runtime/portal diagnostics for commissioning and load visibility.
- Planned outputs:
  - queue metrics struct in runtime snapshot path
  - include queue depth/high-water/drop/latency counters
  - reflect queue telemetry in portal diagnostics payload
