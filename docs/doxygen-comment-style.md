# Doxygen Comment Style Guide (V3)

Date: 2026-03-03  
Status: Active

## 1. Goal

Provide a simple, consistent Doxygen style that covers:

- What a symbol is/does
- Why it exists
- Where it is used
- Critical behavior details (params, return, side effects, timing/context)

This guide is intentionally lightweight so comments stay maintainable.

## 2. Required vs Optional Tags

Required for documented symbols (minimum baseline):

- `@brief`
- `@details`
- `@par Used By`

Optional, add only when relevant to behavior/risk/clarity:

- `@param`, `@return`
- `@pre`, `@post`
- `@sideeffects`
- `@warning`
- `@note`
- `@see`

Policy:

- Do not force every tag on every symbol.
- Comment author decides optional tags based on complexity, change risk, and likely reader confusion points.
- Prefer concise high-signal comments over exhaustive boilerplate.

## 3. Function Template

```cpp
/**
 * @brief <What this function does, one line>.
 * @details <Why this function exists / behavior contract>.
 * @param <name> <Meaning, units/range if relevant>.
 * @param <name> <Meaning>.
 * @return <Return value semantics>.
 * @pre <Required state before call>. (optional)
 * @post <Guaranteed state after call>. (optional)
 * @sideeffects <State mutations, queue ops, I/O>. (optional)
 * @warning <Hazard or common misuse>. (optional)
 * @note <Determinism, allocation, timing, core/task context>. (optional)
 * @par Used By
 * - src/...
 * - src/...
 */
```

## 4. Struct Template

```cpp
/**
 * @brief <What this struct represents>.
 * @details <Why this data contract exists>.
 * @note <Ownership/lifecycle/threading assumptions>. (optional)
 * @par Field Semantics
 * - <field>: <meaning, units/range/default>.
 * - <field>: <meaning>.
 * @par Used By
 * - src/...
 * - src/...
 */
```

## 5. Enum Template

```cpp
/**
 * @brief <What this enum controls>.
 * @details <Why this decision axis exists>.
 * @par Values
 * - <VALUE_A>: <behavior meaning>.
 * - <VALUE_B>: <behavior meaning>.
 * @par Used By
 * - src/...
 * - src/...
 */
```

## 6. Class Template

```cpp
/**
 * @brief <What this class owns>.
 * @details <Why this boundary exists in architecture>.
 * @note <Core/task ownership and determinism policy>. (optional)
 * @par Invariants
 * - <Must always be true>.
 * - <Must always be true>.
 * @par Used By
 * - src/...
 * - src/...
 */
```

## 7. V3-Specific Comment Guidance

1. Use glossary terms from `docs/naming-glossary-v3.md`.
2. Keep `Used By` module-level (2-5 references), not exhaustive call graph.
3. Prefer documenting public `.h` APIs first, then complex `.cpp` internals.
4. For deterministic paths, include timing/allocation notes when relevant.
5. Avoid trivial comments for obvious one-liners.

## 8. Example (V3 Style)

```cpp
/**
 * @brief Evaluate one condition block against runtime signals.
 * @details Centralizes condition semantics so card runtimes stay deterministic.
 * @param block Condition definition (clauseA/clauseB/combiner).
 * @param signals Runtime signal array used for evaluation.
 * @param signalCount Number of valid entries in `signals`.
 * @return `true` when the condition block resolves true.
 * @note Allocation-free and called in scan-cycle path.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 * - src/kernel/v3_payload_rules.cpp
 */
bool evalConditionBlock(const v3::storage::ConditionBlock& block,
                        const V3RuntimeSignal* signals,
                        uint8_t signalCount) const;
```

## 9. Suggested Rollout

1. Phase 1: `src/kernel/*.h`, `src/storage/*.h`, `src/portal/*.h`, `src/runtime/*.h`.
2. Phase 2: critical internal state-machine functions in `.cpp`.
3. Phase 3: remaining high-churn modules as part of normal PRs.

## 10. Doxygen Repo Policy

- Do not commit Doxygen binaries/tools into this repository.
- Commit:
- `Doxyfile` (project config)
- this style guide
- optional generation script (for local/CI use)
- generated docs only if you explicitly want versioned static snapshots.
