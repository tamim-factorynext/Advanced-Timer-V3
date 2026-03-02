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

## 1.1 Skeleton Completion Gate

Do not mark skeleton phase complete until all of these are `DONE`:

- `M18` Real Transport Binding (registered runtime HTTP/WebSocket hooks)
- `M19` Endpoint Command Contract Hardening (final request/response envelope parity)
- `M20` Command Path Observability Parity (transport -> ingress -> control -> kernel correlation fields)
- `M21` Skeleton Freeze Review (code/docs consistency pass + explicit handoff note)

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
- Status: `DONE`
- Date: 2026-03-02
- Summary: expose command/snapshot queue health counters in runtime/portal diagnostics for commissioning and load visibility.
- Implemented outputs:
  - runtime queue telemetry contract:
    - `v3::runtime::QueueTelemetry`
    - file: `src/runtime/runtime_service.h`
  - runtime snapshot wiring:
    - runtime tick now accepts queue telemetry and stores it in snapshot
    - file: `src/runtime/runtime_service.cpp`
  - main loop wiring:
    - Core1 task builds queue telemetry from live queue counters
    - passes telemetry into runtime tick
    - file: `src/main.cpp`
  - portal diagnostics payload:
    - adds `queues.snapshot` and `queues.command` sections
    - includes depth/high-water/drop and command apply/latency counters
    - file: `src/portal/portal_service.cpp`
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:16.524`
  - RAM: `31448 / 327680` (9.6%)
  - Flash: `337501 / 1310720` (25.7%)
- Checkpoint SHA: `d3b1840`

## M13: Real Command DTO Integration (Run/Step Control)
- Status: `DONE`
- Date: 2026-03-02
- Summary: replace no-op heartbeat-only command channel usage with real kernel control commands routed via Core1->Core0 queue.
- Implemented outputs:
  - kernel run/step control hooks:
    - `KernelService::setRunMode(...)`
    - `KernelService::requestStepOnce()`
    - tick behavior honors `RUN_STEP` via pending-step gate
    - files: `src/kernel/kernel_service.h`, `src/kernel/kernel_service.cpp`
  - real command DTO queue usage:
    - Core1 enqueues `KernelCmd_SetRunMode` heartbeat command
    - Core0 dequeues and applies `KernelCmd_SetRunMode` / `KernelCmd_StepOnce`
    - file: `src/main.cpp`
  - command ack telemetry:
    - set-run-mode count
    - step count
    - last applied command type
    - command latency stats
    - files: `src/main.cpp`, `src/runtime/runtime_service.h`
  - portal diagnostics exposure:
    - `queues.command.setRunModeCount`
    - `queues.command.stepCount`
    - `queues.command.lastAppliedType`
    - file: `src/portal/portal_service.cpp`
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:19.293`
  - RAM: `31496 / 327680` (9.6%)
  - Flash: `337789 / 1310720` (25.8%)
- Checkpoint SHA: `d3b1840`

## M14: Command Source Integration (Portal/Control -> Queue)
- Status: `DONE`
- Date: 2026-03-02
- Summary: route real external control intents into Core1 command enqueue path and close the loop with result telemetry.
- Implemented outputs:
  - command dispatch API in control boundary:
    - `ControlService::requestSetRunMode(...)`
    - `ControlService::requestStepOnce(...)`
    - `ControlService::dequeueCommand(...)`
    - `ControlService::diagnostics()`
    - files: `src/control/control_service.h`, `src/control/control_service.cpp`
  - enqueue validation + reject reasons:
    - control-local reject reasons (`QueueFull`, `InvalidRunMode`,
      `StepRequiresRunStep`)
    - control diagnostics counters (requested/accepted/rejected, pending depth/HWM)
  - Core1->kernel dispatch integration:
    - Core1 drains control pending commands and enqueues into kernel queue
    - dispatch queue-full reject counter
    - file: `src/main.cpp`
  - result/ack projection in diagnostics payload:
    - runtime queue telemetry extended with control dispatch metrics
    - portal diagnostics adds `queues.control` section
    - files: `src/runtime/runtime_service.h`, `src/portal/portal_service.cpp`
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:16.651`
  - RAM: `31936 / 327680` (9.7%)
  - Flash: `338601 / 1310720` (25.8%)
- Checkpoint SHA: `d3b1840`

## M15: Portal/API Command Entry Wiring
- Status: `DONE`
- Date: 2026-03-02
- Summary: wire external command entrypoints to control request API and report request-level accept/reject results.
- Implemented outputs:
  - portal command entry API:
    - `enqueueSetRunModeRequest(...)`
    - `enqueueStepOnceRequest(...)`
    - `dequeueCommandRequest(...)`
    - `recordCommandResult(...)`
    - files: `src/portal/portal_service.h`, `src/portal/portal_service.cpp`
  - Core1 request routing:
    - portal request dequeue -> control request API -> result record
    - file: `src/main.cpp`
  - response mapping semantics:
    - per-request accepted/rejected tracking
    - reject reason projection via control reject enum
    - queue-full immediate rejection handling at portal ingress queue
  - diagnostics parity:
    - portal diagnostics adds `commandIngress` section
      (`requested/accepted/rejected/reason/lastRequestId/pending`)
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:13.381`
  - RAM: `32232 / 327680` (9.8%)
  - Flash: `339453 / 1310720` (25.9%)
- Checkpoint SHA: `d3b1840`

## M16: External Command Endpoint Contract (HTTP/WebSocket Hook)
- Status: `DONE`
- Date: 2026-03-02
- Summary: add transport-facing command endpoint hooks to submit portal command requests and return immediate request status with reason mapping.
- Implemented outputs:
  - endpoint hook in portal boundary:
    - `submitSetRunMode(...)`
    - `submitStepOnce(...)`
    - return type: `PortalCommandSubmitResult`
    - files: `src/portal/portal_service.h`, `src/portal/portal_service.cpp`
  - response mapping:
    - immediate submit result includes:
      - `accepted`
      - `reason`
      - `requestId`
    - queue-full path returns rejected with `QueueFull`
  - diagnostics alignment:
    - ingress diagnostics now include:
      - `queueAcceptedCount`
      - `queueRejectedCount`
    - `commandIngress` JSON includes these counters
  - Core1 integration:
    - heartbeat command now enters via `submitSetRunMode(...)`
    - file: `src/main.cpp`
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:13.377`
  - RAM: `32240 / 327680` (9.8%)
  - Flash: `339641 / 1310720` (25.9%)
- Checkpoint SHA: `d3b1840`

## M17: Transport Hook Exposure (HTTP/WebSocket Stub Endpoints)
- Status: `DONE`
- Date: 2026-03-02
- Summary: expose lightweight command submit hooks on transport boundary and return immediate submit result payloads.
- Implemented outputs:
  - transport stub handler module:
    - `src/portal/transport_command_stub.h`
    - `src/portal/transport_command_stub.cpp`
  - command handling:
    - parse command payload JSON
    - supported commands:
      - `setRunMode` with `mode` (`RUN_NORMAL|RUN_STEP|RUN_BREAKPOINT`)
      - `stepOnce`
    - route through portal submit API only:
      - `PortalService::submitSetRunMode(...)`
      - `PortalService::submitStepOnce(...)`
  - transport response mapping:
    - returns JSON body with:
      - `ok`
      - `accepted`
      - `requestId`
      - `reason`
      - `source` (`http|websocket`)
    - status mapping:
      - `200` for accepted submit
      - `429` for rejected submit
      - `400/422` for payload/command validation failures
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:10.470`
  - RAM: `32240 / 327680` (9.8%)
  - Flash: `340773 / 1310720` (26.0%)
- Checkpoint SHA: `d3b1840`

## M18: Real Transport Binding (Endpoint Registration + Handler Wiring)
- Status: `DONE`
- Date: 2026-03-02
- Summary: register concrete HTTP/WebSocket command routes and connect them to transport stub handlers.
- Implemented outputs:
  - new transport runtime module:
    - `src/portal/transport_runtime.h`
    - `src/portal/transport_runtime.cpp`
  - endpoint registration:
    - HTTP `POST /api/v3/command`
    - HTTP `GET /api/v3/diagnostics`
    - WebSocket server on `:81` text command handling
  - payload passthrough:
    - all command payloads routed to `handleTransportCommandStub(...)`
  - response passthrough:
    - HTTP status/body returned from stub response
    - WebSocket response body sent back to requesting client
  - Core1 loop integration:
    - `initTransportRuntime(gPortal)` in setup
    - `serviceTransportRuntime()` in Core1 service loop
    - file: `src/main.cpp`
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:01:21.765`
  - RAM: `37728 / 327680` (11.5%)
  - Flash: `496433 / 1310720` (37.9%)
- Checkpoint SHA: `d3b1840`

## M19: Endpoint Command Contract Hardening
- Status: `DONE`
- Date: 2026-03-02
- Summary: finalize transport command envelope rules and ensure HTTP/WebSocket responses use one canonical schema.
- Implemented outputs:
  - strict response envelope in transport stub:
    - `ok`
    - `accepted`
    - `requestId`
    - `reason`
    - `source`
    - `errorCode`
    - `message`
    - file: `src/portal/transport_command_stub.cpp`
  - normalized status mapping (documented in code comments):
    - `200` accepted submit
    - `429` rejected submit
    - `400` malformed payload/json
    - `422` semantic command validation errors
  - normalized error-code mapping and human-readable messages:
    - parse/shape errors
    - unsupported/invalid command content
    - queue-full and run-mode step constraints
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:14.351`
  - RAM: `37728 / 327680` (11.5%)
  - Flash: `497053 / 1310720` (37.9%)
- Checkpoint SHA: `d3b1840`

## M20: Command Path Observability Parity
- Status: `DONE`
- Date: 2026-03-02
- Summary: add correlation-friendly fields and counters to trace a request from transport submit through kernel apply.
- Implemented outputs:
  - request correlation fields:
    - command DTO now carries portal request id through control -> kernel (`command.value`)
    - kernel apply path stores `kernelLastAppliedRequestId`
    - files: `src/control/control_service.*`, `src/main.cpp`
  - per-stage counters projection in runtime telemetry:
    - portal ingress counters (requested/accepted/rejected/last request/reason)
    - control counters and kernel apply counters
    - file: `src/runtime/runtime_service.h`
  - explicit mismatch flags:
    - `controlRequestedExceedsPortalAccepted`
    - `controlAcceptedExceedsControlRequested`
    - `kernelAppliedExceedsControlAccepted`
    - computed in `src/main.cpp`
  - portal diagnostics parity payload:
    - `queues.portalIngress`
    - `queues.parity`
    - file: `src/portal/portal_service.cpp`
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:17.770`
  - RAM: `38280 / 327680` (11.7%)
  - Flash: `497885 / 1310720` (38.0%)
- Checkpoint SHA: `d3b1840`

## M21: Skeleton Freeze Review
- Status: `DONE`
- Date: 2026-03-02
- Summary: final skeleton closure pass before feature-complete implementation phase.
- Completed outputs:
  - module boundary sanity review (`kernel/runtime/control/storage/portal/platform`):
    - ownership split remains composition-root orchestrated in `src/main.cpp`
    - portal transport isolated in `src/portal/transport_runtime.cpp` and stub handler module
    - config boot ownership remains storage -> kernel validated path
  - queue/task-core ownership review:
    - Core0 owns kernel tick + kernel command apply + snapshot produce
    - Core1 owns portal/control/runtime transport loops and command ingress
    - bounded queue channels in place for command and snapshot paths
  - docs parity pass:
    - `docs/decisions.md`, `docs/worklog.md`, `docs/milestones-v3.md` aligned through `M21`
    - skeleton freeze note added
- Skeleton Freeze Note:
  - Skeleton phase completion gate is satisfied (`M18..M21` all `DONE`).
  - Next phase can begin: feature-complete implementation over stabilized skeleton.

## M22: Watchdog Scaffolding (Per-Task WDT Integration)
- Status: `DONE`
- Date: 2026-03-02
- Summary: wire per-task watchdog primitives and feed points into dual-core task loops.
- Implemented outputs:
  - platform watchdog primitives:
    - `initTaskWatchdog(...)`
    - `addCurrentTaskToWatchdog()`
    - `resetTaskWatchdog()`
    - files: `src/platform/platform_service.h`, `src/platform/platform_service.cpp`
  - task integration:
    - initialize watchdog in setup with timeout policy
    - register Core0/Core1 tasks
    - feed watchdog in both task loops
    - file: `src/main.cpp`
- Remaining:
  - none
- HIL Validation Note:
  - During HIL, manually trigger watchdog timeout (for example, intentionally stop watchdog feed in one task) to verify reset/recovery path and diagnostics behavior.
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:15.634`
  - RAM: `38280 / 327680` (11.7%)
  - Flash: `498249 / 1310720` (38.0%)
- Checkpoint SHA: `d3b1840`

## M23: Config-Driven WiFi Policy Wiring (Master -> User -> Offline)
- Status: `DONE`
- Date: 2026-03-02
- Summary: replace hardcoded WiFi credentials/timeouts with validated config policy and enforce STA-only dual-SSID strategy in runtime.
- Implemented outputs:
  - extended storage config contract with WiFi fields:
    - `wifi.master { ssid, password, timeoutSec, editable }`
    - `wifi.user { ssid, password, timeoutSec, editable }`
    - `wifi.retryBackoffSec`
    - `wifi.staOnly`
    - files:
      - `src/storage/v3_config_contract.h`
      - `src/storage/v3_config_contract.cpp`
  - decoder support for optional `wifi` object over default policy:
    - parse + bounds-check credential strings
    - parse timeouts/backoff/staOnly
    - file: `src/storage/v3_config_decoder.cpp`
  - validator rules for WiFi policy:
    - reject `staOnly=false`
    - reject `master.editable=true`
    - reject empty SSIDs/passwords
    - reject zero timeout/backoff values
    - file: `src/storage/v3_config_validator.cpp`
  - runtime wiring to consume validated config policy:
    - `WiFiRuntime::begin(const WiFiConfig&)`
    - `main.cpp` now calls `gWiFi.begin(gStorage.activeConfig().system.wifi)`
    - files:
      - `src/platform/wifi_runtime.h`
      - `src/platform/wifi_runtime.cpp`
      - `src/main.cpp`
- Remaining:
  - none
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
  - Duration: `00:00:29.027`
  - RAM: `58596 / 327680` (17.9%)
  - Flash: `868349 / 1310720` (66.2%)
