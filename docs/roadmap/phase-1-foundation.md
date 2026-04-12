# Phase 1 Foundation Roadmap

## Scope
Phase 1 establishes a deterministic raid core, rollback-safe networking behavior, and a clear contributor on-ramp.

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
| P1-RAID-B | `Source/raid/raid_rules.*` | `raid_sync_test`, `raid_protocol_test` | Lockout and eligibility checks are deterministic across host/client simulation, including late join/reconnect/stale cache denial cases. | Raid Systems Lead |
| P1-RAID-C | `Source/raid/raid_progression.*` | `raid_progression_test`, `raid_protocol_test` | Rewards are granted exactly once per canonical completion key and remain deduplicated through reconnect/replay/rollback paths. | Raid Systems Lead |
| P1-ENCOUNTER-A | `Source/raid/encounter_schema.*` | `encounter_schema_test` | Encounter and loot schema validation accepts valid multi-reward payloads and rejects malformed reward entries with deterministic outcomes. | Raid Systems Lead |
| P1-ENCOUNTER-B | `Source/raid/encounter_engine_phase.*` | `encounter_engine_phase_test` | Scripted boss phase flow obeys explicit next-phase links and condition gates, including deterministic fail-state behavior. | Raid Systems Lead |
| P1-NET-A | `Source/dvlnet/rollback_state.*` | `rollback_state_test`, `raid_sync_test` | Divergence detection remains deterministic and monotonic per frame with explicit soft/hard thresholds under synchronized state slices. | Networking Lead |
| P1-NET-B | `Source/dvlnet/rollback_state.*` | `rollback_state_test` | Correction replay is idempotent, preserves immutable baselines, and never advances authoritative frame beyond validated bounds. | Networking Lead |
| P1-NET-C | `Source/dvlnet/net_telemetry.*` | `net_telemetry_trace_test` | Telemetry lines round-trip without semantic drift, malformed fields produce structured errors, and versioned fields remain backward-compatible. | Networking Lead |

---

## Track 2: Netcode Integrity

### Targets
- `Source/dvlnet/rollback_state.*`
- `Source/dvlnet/net_telemetry.*`

### Invariants

#### A) Divergence detection (`rollback_state.*`)
- Divergence is computed from deterministic frame fingerprints over synchronized state slices.
- Detection must be monotonic per frame: once marked divergent, frame remains divergent until corrected.
- Thresholds for soft vs hard divergence are explicit and test-covered.

#### B) Correction replay safety (`rollback_state.*`)
- Replay never mutates immutable baseline snapshots.
- Replay is idempotent for identical correction payloads.
- Replay cannot advance authoritative frame counter beyond validated correction bounds.
- Safety tests include out-of-order corrections, duplicate corrections, and partial state deltas.

#### C) Telemetry parse/format consistency (`net_telemetry.*`)
- Every emitted telemetry line round-trips (`format -> parse -> format`) without semantic drift.
- Parser rejects malformed fields with structured error tags (not silent fallback).
- Versioned telemetry schema fields remain backward-compatible for prior minor versions.
- Tests include fuzzed line inputs and golden-line fixtures.

---

## Track 3: Dev Experience

### Contributor entry map
- Start: `README.md` (project orientation, build prerequisites).
- Build details: `docs/building.md` (platform-specific setup and build invocations).
- Key tests to run first:
  - `test/raid_state_test.cpp`
  - `test/raid_progression_test.cpp`
  - `test/raid_protocol_test.cpp`
  - `test/raid_sync_test.cpp`
  - `test/rollback_state_test.cpp`

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
