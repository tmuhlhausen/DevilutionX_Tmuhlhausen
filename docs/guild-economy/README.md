# Guild Economy & Rewards

## Scope and goals

This document defines the delivery plan for the guild economy system: treasury accounting, weekly rewards, inflation control, and admin-facing reconciliation tooling.

Goals:
- Keep guild ledger accounting deterministic and auditable.
- Ensure reward logic is fully test-covered before rollout.
- Add measurable balancing telemetry to control currency inflation.
- Make reconciliation fast enough for practical weekly operations.

## Acceptance criteria mapped from README.md roadmap bullets

| Roadmap bullet (README.md) | Acceptance criteria | Evidence |
|---|---|---|
| Introduce guild treasury transaction types with invariant checks that keep ledger drift at `0` in nightly audit tests. | Transaction schemas are versioned; invariants validate balance conservation; nightly audit suite reports zero drift for all seeded guild scenarios. | `test/guild_economy_audit_test.cpp` and nightly CI artifact with drift summary. |
| Implement weekly reward distribution rules with automated tests covering `100%` of payout branches and edge cases (join/leave mid-cycle, ties, inactivity). | Reward engine captures all payout branches with statement/branch coverage gates at 100% for payout module. Edge-case fixtures are required for join/leave, ties, inactivity. | `test/guild_rewards_test.cpp` coverage report + edge-case fixture set. |
| Add anti-inflation sinks and balancing telemetry, targeting a monthly net currency delta within `±5%` of design forecast. | Sinks are configurable and gated by feature flags; telemetry exports monthly delta against design baseline; alert triggers if outside ±5%. | Monthly telemetry export + alert log in `docs/ops/`. |
| Provide guild progression dashboards and event exports so admins can reconcile reward outcomes in under `10 minutes` per cycle. | Dashboard includes progression and payout drill-down; event export is machine-readable; user acceptance runbook demonstrates reconciliation within 10 minutes. | Reconciliation runbook + benchmark notes in docs. |

## Required tests and file ownership

Required tests:
- Deterministic treasury invariant tests.
- Reward payout branch/edge-case tests.
- Inflation telemetry regression tests.
- End-to-end reconciliation scenario test.

Suggested ownership:
- Primary: Gameplay Systems + Economy maintainers.
- Secondary: QA Automation for deterministic/nightly coverage.
- Docs owner: `docs/guild-economy/` maintainers for runbooks and acceptance evidence.

## Status

| Capability | Scaffold Completed | Feature Operational | Notes |
|---|---|---|---|
| Treasury transactions & invariants | ✅ | ⛔ | Documentation scaffolded; implementation/tests pending. |
| Weekly reward distribution rules | ✅ | ⛔ | Acceptance matrix defined; code and coverage gating pending. |
| Anti-inflation sinks & telemetry | ✅ | ⛔ | Target thresholds documented; instrumentation pending. |
| Dashboards & reconciliation exports | ✅ | ⛔ | Operational workflow documented; tooling pending. |
