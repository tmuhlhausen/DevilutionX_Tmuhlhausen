# Phase 1 Foundation Roadmap

## Scope
Phase 1 establishes a deterministic raid core, rollback-safe networking behavior, and a clear contributor on-ramp.

## Current code map (canonical netcode locations)

- **Current netcode (authoritative now):** `Source/dvlnet/*`.
- **Future migration target (planned only unless explicitly landed):** `Source/engine/net/*`.

### Net migration stage legend
- **Stage 0 — Current-only (`dvlnet`)**: Acceptance must validate behavior in `Source/dvlnet/*`.
- **Stage 1 — Dual-path (`dvlnet` + `engine/net`)**: `Source/engine/net/*` may be introduced, but completion still requires `dvlnet` parity.
- **Stage 2 — Engine-net authoritative (`engine/net`)**: `Source/engine/net/*` is canonical; `dvlnet` is compatibility fallback.

> Completion guardrail: Netcode tasks cannot be marked complete against `Source/engine/net/*` alone unless the task explicitly requires Stage 1 or Stage 2.

## Track 1: Raid Core

### Targets
- `Source/raid/raid_state.*`
- `Source/raid/raid_rules.*`
- `Source/raid/raid_progression.*`

### Raid acceptance traceability (README targets)

| Acceptance target | Acceptance check | Test coverage |
|---|---|---|
| boss phases | Scripted boss phase progression uses explicit next-phase links and condition-gated transitions. | `EncounterEnginePhaseTest.ScriptedTransitionUsesNextPhaseId`, `EncounterEnginePhaseTest.MechanicConditionBlocksUntilSatisfied` |
| loot tables | Loot-table schema accepts valid multi-reward payloads and rejects malformed reward entries. | `EncounterSchemaTest.LootTableAcceptsMultipleDistinctRewards`, `EncounterSchemaTest.LootTableRejectsEmptyIdAndZeroQuantity` |
| fail states | Boss fail states emit deterministic failure events and enforce rollback policy behavior. | `EncounterEnginePhaseTest.WipeFailureEmitsFailStateAndTracksWipes`, `EncounterEnginePhaseTest.CheckpointRollbackFailureReturnsToStartPhase` |

### Acceptance checks

#### A) State transition correctness (`raid_state.*`)
- Valid transitions only: `Lobby -> Countdown -> Active -> Complete` and `* -> Aborted` (where allowed by rules).
- Invalid transitions are rejected and logged with reason codes.
- Transition replay from authoritative snapshots yields identical terminal state.
- Regression tests cover happy-path, abort-path, and invalid-edge transitions.

#### B) Lockout enforcement (`raid_rules.*`)
- Account/character lockout windows are consistently applied before raid entry.
- Lockout bypass attempts (late join, reconnect, stale client cache) are denied deterministically.
- Lockout expiration is time-source consistent between host and client simulation.
- Rule-evaluation tests validate lockout and eligibility matrixes.

#### C) Reward deduplication (`raid_progression.*`)
- Rewards are keyed by stable completion identity (`raid_id + encounter_id + completion_nonce` or equivalent canonical key).
- Duplicate completion packets/events cannot mint duplicate rewards.
- Reconnect/replay paths preserve exactly-once reward grant semantics.
- Progression tests verify first-grant, duplicate-event, and rollback/replay scenarios.

### Acceptance traceability table

| Acceptance check ID | Source module | Test target(s) from `CMake/Tests.cmake` | Pass condition | Owner |
|---|---|---|---|---|
| P1-RAID-A | `Source/raid/raid_state.*` | `raid_state_test`, `raid_sync_test` | Transition graph only permits valid edges, rejects invalid edges with reason codes, and snapshot replay lands in identical terminal state. | Raid Systems Lead |
| P1-RAID-B | `Source/raid/raid_rules.*` | `raid_protocol_test`, `raid_sync_test` | Lockout and eligibility checks are deterministic across host/client simulation, including late join/reconnect/stale cache denial cases. | Raid Systems Lead |
| P1-RAID-C | `Source/raid/raid_progression.*` | `raid_progression_test`, `raid_protocol_test` | Rewards are granted exactly once per canonical completion key and remain deduplicated through reconnect/replay/rollback paths. | Raid Systems Lead |
| P1-ENCOUNTER-A | `Source/raid/encounter_schema.*` | `encounter_schema_test` | Encounter and loot schema validation accepts valid multi-reward payloads and rejects malformed reward entries with deterministic outcomes. | Raid Systems Lead |
| P1-ENCOUNTER-B | `Source/raid/encounter_engine_phase.*` | `encounter_engine_phase_test` | Scripted boss phase flow obeys explicit next-phase links and condition gates, including deterministic fail-state behavior. | Raid Systems Lead |
| P1-NET-A | `Source/dvlnet/rollback_state.*` | `rollback_state_test`, `raid_sync_test` | Divergence detection remains deterministic and monotonic per frame with explicit soft/hard thresholds under synchronized state slices. | Networking Lead |
| P1-NET-B | `Source/dvlnet/rollback_state.*` | `rollback_state_test`, `net_chaos_test` | Correction replay is idempotent, preserves immutable baselines, and never advances authoritative frame beyond validated bounds under duplicates/out-of-order deltas. | Networking Lead |
| P1-NET-C | `Source/dvlnet/net_telemetry.*` | `net_telemetry_trace_test`, `net_qos_test` | Telemetry lines round-trip without semantic drift, malformed fields produce structured errors, and versioned fields remain backward-compatible under QoS pressure. | Networking Lead |

---

## Track 2: Netcode Integrity

### Targets
- `Source/dvlnet/rollback_state.*`
- `Source/dvlnet/net_telemetry.*`

### Invariants

#### A) Divergence detection (`rollback_state.*`)
- Divergence is computed from deterministic frame fingerprints over synchronized state slices. *(Path: `dvlnet`, Stage: 0)*
- Detection must be monotonic per frame: once marked divergent, frame remains divergent until corrected. *(Path: `dvlnet`, Stage: 0)*
- Thresholds for soft vs hard divergence are explicit and test-covered. *(Path: `dvlnet` now, `engine/net` at Stage 1)*

#### B) Correction replay safety (`rollback_state.*`)
- Replay never mutates immutable baseline snapshots. *(Path: `dvlnet`, Stage: 0)*
- Replay is idempotent for identical correction payloads. *(Path: `dvlnet`, Stage: 0)*
- Replay cannot advance authoritative frame counter beyond validated correction bounds. *(Path: `dvlnet`, Stage: 0)*
- Safety tests include out-of-order corrections, duplicate corrections, and partial state deltas. *(Path: `dvlnet` tests now, `engine/net` duplication allowed at Stage 1)*

#### C) Telemetry parse/format consistency (`net_telemetry.*`)
- Every emitted telemetry line round-trips (`format -> parse -> format`) without semantic drift. *(Path: `dvlnet`, Stage: 0)*
- Parser rejects malformed fields with structured error tags (not silent fallback). *(Path: `dvlnet`, Stage: 0)*
- Versioned telemetry schema fields remain backward-compatible for prior minor versions. *(Path: `dvlnet` now, `engine/net` at Stage 1+)*
- Tests include fuzzed line inputs and golden-line fixtures. *(Path: `dvlnet` tests now, mirrored `engine/net` coverage required at Stage 1+)*

---

## Track 3: Dev Experience

### Contributor entry map
- Start: `README.md` (project orientation, build prerequisites).
- Build details: `docs/building.md` (platform-specific setup and build invocations).
- Key tests to run first:
  - `test/raid_state_test.cpp`
    - Why this test matters (Track 1 / Acceptance A): Validates authoritative raid lifecycle transitions, invalid-edge rejection, and replay-safe terminal state convergence.
  - `test/raid_progression_test.cpp`
    - Why this test matters (Track 1 / Acceptance C): Verifies reward grant identity/dedup logic across first-grant, duplicate, reconnect, and replay paths.
  - `test/raid_protocol_test.cpp`
    - Why this test matters (Track 1 / Acceptance A-B): Confirms deterministic protocol handling for raid state/rules decisions and lockout-sensitive entry flows.
  - `test/raid_sync_test.cpp`
    - Why this test matters (Track 1 / Acceptance A): Ensures host/client raid synchronization preserves deterministic transition outcomes.
  - `test/encounter_schema_test.cpp`
    - Why this test matters (Track 1 / Acceptance C): Guards encounter + loot schema validity so malformed reward payloads cannot bypass dedup-safe progression rules.
  - `test/encounter_engine_phase_test.cpp`
    - Why this test matters (Track 1 / Acceptance A): Validates scripted boss phase transitions, condition gating, and deterministic fail/rollback behavior.
  - `test/rollback_state_test.cpp`
    - Why this test matters (Track 2 / Invariants A-B): Covers divergence detection monotonicity and correction replay safety/idempotence boundaries.
  - `test/net_telemetry_trace_test.cpp`
    - Why this test matters (Track 2 / Invariant C): Confirms telemetry trace parse/format round-trip consistency and structured handling of malformed data.
  - `test/net_chaos_test.cpp`
    - Why this test matters (Track 2 / Invariant B): Stress-tests rollback correction behavior under reordering/loss-like chaos to protect replay safety guarantees.
  - `test/net_qos_test.cpp`
    - Why this test matters (Track 2 / Invariants A-C): Validates QoS classification/handling so divergence signals and telemetry remain actionable under network pressure.

### DX outcomes
- New contributors can move from clone to first targeted test run with a single, documented path.
- Raid and rollback tests are discoverable from docs without searching the tree.
- Test naming and ownership are visible from roadmap artifacts.

---

## Milestone checklist (Phase 1)

| Track | Owner | ETA | Definition of done | Status |
|---|---|---:|---|---|
| Raid Core | Raid Systems Lead | 2026-05-15 | All Raid Core acceptance checks automated; CI green on raid suite; no known reward-dup or lockout-severity bugs open. | ☐ |
| Netcode Integrity | Networking Lead | 2026-05-29 | All netcode invariants enforced with tests; rollback correction replay validated under stress; telemetry round-trip suite green. | ☐ |
| Dev Experience | Docs + Tooling Lead | 2026-05-08 | Contributor entry map merged; docs link to key tests; first-time contributor dry-run completed and documented. | ☐ |

### Cross-track exit criteria
- [ ] Owners assigned and acknowledged in planning sync.
- [ ] ETA confidence >= 80% with explicit risk notes.
- [ ] CI gate includes required raid + rollback test subsets.
- [ ] Phase 1 sign-off recorded in engineering notes.
