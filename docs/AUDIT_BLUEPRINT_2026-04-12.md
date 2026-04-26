# Repository Audit Blueprint (2026-04-12)

## Scope

This audit pass reviews repository health at a high level and defines a **massive-addition roadmap** with minimal removals.

Focus areas:
- CI / quality gates
- Core-engine technical debt hotspots
- Test strategy and determinism
- Security and supply-chain posture
- Docs and contributor velocity

## What was measured in this pass

- Repository file inventory indicates **1472 tracked files**.
- C/C++ code inventory under `Source/` and `test/` indicates **711 source/header files**.
- `TODO` / `FIXME` markers in `Source/` indicate **18 known debt markers**.

## Priority findings

### 1) CI is broad and multi-platform (strong baseline)

The project already runs broad CI matrix coverage and explicit formatting/tidy workflows, which is a strong platform-quality baseline.

### 2) Several TODO/FIXME markers indicate correctness and maintainability risks

Notable examples include:
- Unused/legacy fields that should be removed when safe.
- Potential infinite-loop risk when generating items.
- Fallback behavior returning `{0,0}` on item-drop positioning.
- Cross-dialog variable coupling in UI flows.

These are manageable, but they are concentrated in gameplay-critical paths and UI coordination code.

### 3) Determinism and regression strategy is promising

The repo has substantial test coverage scaffolding (including raid/guild/net-related tests), but it would benefit from stricter audit-style gating tied to release criteria.

## Revolutionary add-first roadmap (no destructive churn)

## Wave 1 — “Always-Trust Build” (2–3 weeks)

1. Add `docs/audit/` with machine-generated quality snapshots committed per release:
   - static TODO/FIXME trend report
   - flaky-test tracker
   - platform success-rate table
2. Add a CI meta-job that fails on:
   - TODO/FIXME count growth in `Source/` (unless allowlisted)
   - coverage regression over threshold
3. Add reproducible local script `tools/audit/run_audit.sh` that mirrors the CI audit checks.

## Wave 2 — “Deterministic Core” (3–5 weeks)

1. Add property/fuzz tests for item generation, drop placement, and monster spawn edge cases.
2. Add deterministic replay seeds for top 20 crash-prone gameplay flows.
3. Introduce an invariant-check layer for risky structs and economy/event transactions.

## Wave 3 — “Security & Supply Chain Hardening” (2–4 weeks)

1. Add SBOM generation + dependency review gate in CI.
2. Add secret-scanning and basic static security scan job.
3. Add signed release provenance + artifact verification docs for contributors.

## Wave 4 — “Contributor Velocity at Scale” (ongoing)

1. Add `good-first-audit` issue labels auto-generated from TODO/FIXME hotspots.
2. Add per-subsystem ownership map in docs (`core`, `render`, `network`, `ui`, `platform`).
3. Add an “Audit Dashboard” doc updated every milestone with objective metrics.

## Suggested KPI scoreboard

Track these every release:
- CI pass rate per platform target: `>= 98%`
- TODO/FIXME in `Source/`: non-increasing trend
- Crash/desync reproduction rate in deterministic replay suite
- Mean time to merge for medium PRs
- Coverage delta versus previous release

## Immediate low-risk starter tasks

1. Create `tools/audit/run_audit.sh` and wire it into one Linux test workflow.
2. Add `docs/audit/baseline.md` with first KPI snapshot.
3. Open issues for the high-risk TODO/FIXME hotspots listed in this blueprint.

## Notes

This blueprint is intentionally additive: prioritize automation, observability, and safety rails before deep refactors.
