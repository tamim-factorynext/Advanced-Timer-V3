# Naming Glossary And Migration Plan V3

Date: 2026-03-03  
Status: Planned (pre-code-rename baseline)

## 1. Purpose

Define one user-first naming vocabulary for portal, API, config, and diagnostics so behavior is self-explanatory and consistent across the product.

## 2. Naming Rules

- Prefer behavior names over implementation names.
- Prefer user language over firmware shorthand.
- Include unit suffixes (`Ms`, `Sec`, `Pct`) where relevant.
- Use one canonical term per concept across docs/API/UI.
- Keep backward compatibility aliases during migration.

## 3. Family Naming (User-Facing Labels)

| Current Token | Canonical User Label | Allowed Alternate Labels |
| --- | --- | --- |
| `DI` | Digital Input | Input Sensor, Contact Input |
| `DO` | Digital Output | Output Relay, Actuator Output |
| `AI` | Analog Input | Analog Sensor, Process Input |
| `SIO` | Virtual Output | Virtual Relay, Soft Relay |
| `MATH` | Computation | Calculation, Formula |
| `RTC` | Schedule Alarm | Scheduler, Time Alarm |

## 4. Runtime State Naming

| Current Key | Canonical Key | Rationale |
| --- | --- | --- |
| `commandState` | `commandState` | "Commanded/evaluated state" is clearer for users. |
| `actualState` | `actualState` | Represents real line/output state. |
| `edgePulse` | `edgePulse` | One-scan event pulse semantics are explicit. |
| `liveValue` | `liveValue` | Generic runtime value for UI display. |
| `state` | `missionState` | Avoid vague generic `state`. |
| `mode` | `operationMode` | Distinguishes card behavior mode. |
| `repeatCounter` | `cycleCount` | User-readable loop count term. |
| `startOnMs` | `onDelayMs` | Describes timing behavior directly. |
| `startOffMs` | `onTimeMs` | Describes active/on window duration. |
| `turnOnConditionMet` | `turnOnConditionMet` | Boolean meaning is explicit. |
| `turnOffConditionMet` | `turnOffConditionMet` | Boolean meaning is explicit. |

## 5. Condition Language

| Current Term | Canonical Term | Notes |
| --- | --- | --- |
| `set` | `turnOnCondition` | Explicit boolean-condition intent with minimal ambiguity. |
| `reset` | `turnOffCondition` | Explicit boolean-condition intent with minimal ambiguity. |
| `ConditionBlock` | `RuleBlock` | Portal-level readability. |
| `clauseA` | `rule1` | Avoid letter-only semantics. |
| `clauseB` | `rule2` | Avoid letter-only semantics. |
| `combiner` | `ruleJoin` | Explicit join/logic purpose. |
| `sourceCardId` | `sourceId` | Cleaner key for UI/API. |
| `thresholdValue` | `compareValue` | Constant numeric compare value for clause RHS. |
| `thresholdCardId` | `compareFromCardId` | Card reference used when compare source is card `liveValue`. |
| `useThresholdCard` | `useCardCompareValue` | Boolean mode switch between constant and card-reference RHS. |
| `missionState` | `phase` | Simpler user-facing wording for DO/SIO phase state. |

## 5.1 Live Badge Token Vocabulary (Frozen)

These tokens are for compact Live page badge rendering and Card Wizard preview text.

| Signal Source | Token Pair | Example |
| --- | --- | --- |
| `commandState` | `ON` / `OFF` | `DI1:ON` |
| `actualState` | `HIGH` / `LOW` | `DO0:HIGH` |
| `edgePulse` | `TRIG` / `CLR` | `Alarm0:TRIG` |
| `missionState` | `IDLE` / `RUN` / `DONE` | `SIO0:RUN` |
| constant clause | `TRUE` / `FALSE` | `DO2:TRUE` |

Badge expression format rules:

- Primary-only combiner: render only clause A text.
- AND combiner: render as `A & B`.
- OR combiner: render as `A | B`.
- Do not render explicit combiner labels (`[AND]`, `[OR]`, `[NONE]`).
- Badge text is static expression; badge color reflects runtime evaluation result.

## 6. AI/MATH Naming

| Current Key | Canonical Key | Notes |
| --- | --- | --- |
| `smoothingFactorPct` | `smoothingFactorPct` | Replaces algorithm term with user-facing intent. |
| `fallbackValue` | `safeValue` | Error/invalid-path output is clear. |
| `inputMin` | `rawMin` | Distinguishes from scaled range. |
| `inputMax` | `rawMax` | Distinguishes from scaled range. |
| `outputMin` | `scaledMin` | Engineering/scaled range endpoint. |
| `outputMax` | `scaledMax` | Engineering/scaled range endpoint. |
| `operation` | `operation` | Keep explicit operator selection (`add/sub/mul/div`) for dropdown UX. |

## 7. Time Sync Naming

Preferred user-facing concept label:

- `Time / Time Sync Server`

| Current Key | Canonical Key | Notes |
| --- | --- | --- |
| `clock` | `time` | Use user-facing domain term `time`. |
| `timezone` | `timeZone` | Consistent casing/readability. |
| `ntp` | `timeSync` | Replace protocol acronym in UI/API contract. |
| `enabled` | `enabled` | Keep unchanged. |
| `primaryTimeServer` | `primaryTimeServer` | Explicit purpose. |
| `syncIntervalSec` | `syncEverySec` | Reads as schedule. |
| `startupTimeoutSec` | `startupWaitSec` | Simpler wording. |
| `maxStaleSec` | `maxTimeAgeSec` | User can infer stale-time policy. |

## 8. Settings Naming (System UX)

| Current Key | Canonical Key | Notes |
| --- | --- | --- |
| `scanPeriodMs` | `scanPeriodMs` | Period language is clearer in settings. |
| `engineMode` | `engineMode` | Distinguishes global run mode from card mode. |
| `snapshot` | `liveState` | User-facing runtime state payload term. |
| `diagnostics` | `systemHealth` | Operator-friendly troubleshooting term. |
| `queueHighWater` | `queuePeakDepth` | Peak depth is directly understandable. |
| `dropCount` | `droppedCount` | Clear event-loss count term. |

## 9. WiFi Settings Naming

| Current Key | Canonical Key | Notes |
| --- | --- | --- |
| `master` | `backupAccessNetwork` | Backup access path for recovery/commissioning. |
| `user` | `userConfiguredNetwork` | Primary user-provided network profile. |
| `timeoutSec` | `connectTimeoutSec` | Connect operation timeout. |
| `retryDelaySec` | `retryDelaySec` | Retry wait interval. |
| `staOnly` | `staOnly` | Keep unchanged. |
| `editable` | `userEditable` | Explicit permission semantics. |

## 10. Migration Plan

Single-step plan (firmware-first, no dual-key compatibility):

1. Rename canonical keys and symbols across firmware modules first (`storage`, `kernel`, `runtime`, `portal` backend payload builders).
2. Update contract docs in the same change set (`api-contract`, `schema`, `user-guide`) to the new names only.
3. Keep portal implementation aligned to the renamed firmware contract when portal work starts.
4. Treat this as a breaking terminology update in internal notes/changelog.

## 11. Guardrails

- Never rename deterministic semantics without updating acceptance matrix references.
- Update `docs/api-contract-v3.md` and `docs/schema-v3.md` in the same change set as code rename.
- Add focused regression tests for renamed keys and updated payload parsing/serialization.
- Avoid mixed vocabulary in the same payload section.

## 12. Scope For Next Code Mission

- `src/storage/` decode/validator/config contract key aliases.
- `src/portal/` snapshot/diagnostics serialization keys.
- `src/kernel/` enum/string mapping where exposed externally.
- `docs/api-contract-v3.md`, `docs/schema-v3.md`, `docs/user-guide-v3-draft.md`.




