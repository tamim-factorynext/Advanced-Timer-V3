# RAM-First Migration Plan (In-Place, No Branch)

Date: 2026-03-05  
Status: Active planning baseline for SRAM-constrained V3 migration.

## 1. Goal

Rebuild memory ownership around ESP32 internal SRAM constraints (including contiguous allocation pressure) by replacing monolithic JSON and duplicated runtime buffers with:

- one global settings contract
- one card-index contract
- one per-card contract
- compact runtime state and delta-style snapshot transport

This migration is executed **in-place on main** with strict deletion gates to avoid long-lived dual-path complexity.

## 2. Non-Negotiable Architecture Targets

1. Storage split is mandatory:
- `settings.json` (system-level settings only)
- `cards/index.json` (card ids/order/revision metadata)
- `cards/<id>.json` (single-card config record)

2. Runtime must not depend on JSON layout:
- storage layer owns file/JSON parsing and validation
- kernel/runtime consume compact typed runtime descriptors only

3. Transport split must mirror storage split:
- settings endpoints separate from card endpoints
- snapshot transport uses small global metrics + per-card update frames

4. No persistent monolithic config/snapshot builder in hot path:
- large all-cards JSON payloads are migration/import/debug only
- not used for normal runtime loop or frequent API refresh

## 3. Anti-Mangle Rules

1. Single compatibility seam only:
- legacy monolithic format allowed only as one-way importer during cutover
- importer converts to new files, then runtime writes new format only

2. No mixed ownership:
- kernel/runtime modules cannot read raw JSON
- portal cannot directly inspect storage file structure

3. Deletion gate after each phase:
- transitional adapters introduced in a phase must be removed by that phase exit criteria unless explicitly listed as temporary in this document

4. Feature freeze during migration:
- no unrelated feature additions until Phase 5 completes

## 4. Phase Plan

## Phase 0: Baseline + Guardrails
Entry:
- this document accepted as working plan

Actions:
- establish memory budget table (static, stacks, steady heap, peak heap)
- add runtime telemetry for heap/stack high-water and allocation failures
- define acceptance thresholds for scan jitter and snapshot latency

Exit:
- measurable memory and timing baselines captured
- fail criteria documented for each threshold

## Phase 1: Storage Contract Split
Entry:
- Phase 0 complete

Actions:
- implement canonical new layout:
  - `settings.json`
  - `cards/index.json`
  - `cards/<id>.json`
- implement boot importer from old monolithic file to new split layout
- implement atomic write policy (temp file -> close -> rename) per artifact
- add manifest/version checks for partial-write detection and safe recovery

Exit:
- boot works with both old and new on first migration
- post-migration writes occur only in new split layout
- monolithic writer path deleted

## Phase 2: Typed Compile Layer (Storage -> Runtime)
Entry:
- Phase 1 complete

Actions:
- compile validated settings + card files into compact runtime descriptors
- remove all-family-overhead from hot runtime structures where possible
- size runtime arrays by active profile/card counts, not blanket max where feasible

Exit:
- runtime boot path consumes compact compiled descriptors only
- JSON structures not retained in kernel-hot memory

## Phase 3: Transport/API Split + Snapshot Delta Model
Entry:
- Phase 2 complete

Actions:
- API surface split:
  - settings read/write
  - card index read
  - per-card read/write
- snapshot channel redesign:
  - small global metrics frame
  - per-card state frames (delta-first, heartbeat fallback)
  - sequence/revision ids for resync detection
- keep optional full-export endpoint for diagnostics only

Exit:
- periodic runtime monitoring no longer requires full all-cards JSON serialization
- frontend can render from normalized card cache + metrics stream

## Phase 4: Frontend Data Model Migration
Entry:
- Phase 3 complete

Actions:
- normalize UI store by card id
- switch from full snapshot replace to incremental card updates
- lazy-load heavy card detail when required

Exit:
- UI does not require monolithic payload for normal interaction
- reconnect/resync behavior verified with sequence mismatch recovery

## Phase 5: Legacy Removal + Stabilization
Entry:
- Phase 4 complete

Actions:
- remove monolithic importer after migration window closes
- remove temporary adapters/shims
- run soak tests and recovery tests (power loss during write, reconnect, boot recovery)

Exit:
- one canonical path only (split storage + delta transport)
- no legacy JSON builder/reader in production runtime paths

## 5. Data Contracts (Target Shape)

`settings.json`:
- system/global settings only (wifi/time/runtime policy/etc.)
- no per-card config fields

`cards/index.json`:
- schema/version
- ordered card id list
- per-card revision summary

`cards/<id>.json`:
- single-card family + params only
- no global settings duplication

Snapshot frames:
- `metrics` frame (global counters/timing/queue health)
- `card_update` frame (id + changed state fields + sequence)
- periodic low-rate heartbeat frame for liveness

## 6. Acceptance Gates

Memory gates:
- no large contiguous runtime JSON buffer reservation in steady state
- stable heap behavior under sustained snapshot traffic
- stack high-water remains above agreed safety margin in both cores

Behavior gates:
- deterministic scan timing remains within threshold
- config write remains atomic and recoverable
- frontend state stays consistent across dropped/out-of-order updates

Cleanup gates:
- no parallel old/new write paths after Phase 1
- no temporary adapter survives past declared expiry phase

## 7. Heap/Stack Governance (Mandatory)

1. Heap policy:
- no unbounded `String` growth in periodic paths
- no repeated allocate/free churn in high-frequency loops
- pre-reserve only small bounded buffers where unavoidable
- large payload generation must be streamed/chunked

2. Stack policy:
- no large local structs/arrays in task loops
- move heavy temporaries to static bounded buffers or compact shared work areas
- every task must log and monitor stack high-water in runtime diagnostics

3. Ownership policy:
- kernel-hot path uses fixed-size deterministic memory only
- control/portal paths may use heap but must stay within declared budget
- JSON DOM-style full-tree construction is forbidden for large payloads in steady-state runtime

4. Verification policy:
- track minimum free heap and largest free block over time
- track per-task stack high-water marks
- treat budget regression as migration failure, not “later cleanup”

## 8. Execution Order (Recommended)

1. Phase 0 and Phase 1 in one focused pass (storage foundation first).
2. Phase 2 and Phase 3 next (runtime + transport boundary alignment).
3. Phase 4 and Phase 5 final (UI + legacy removal and soak).

This sequence gives the speed of decisive migration without leaving mixed architecture debt in the codebase.
