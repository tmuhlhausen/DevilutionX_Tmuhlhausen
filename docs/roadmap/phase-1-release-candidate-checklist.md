# Phase 1 Release-Candidate Checklist

Purpose: provide a single pre-release gate for Phase 1 that is **additive-only** and directly traceable to the roadmap and test definitions.

## 1) Exact Phase-1 acceptance IDs

Mark each item complete only when its pass condition is met and evidence is attached.

- [ ] `P1-RAID-A`
- [ ] `P1-RAID-B`
- [ ] `P1-RAID-C`
- [ ] `P1-ENCOUNTER-A`
- [ ] `P1-ENCOUNTER-B`
- [ ] `P1-NET-A`
- [ ] `P1-NET-B`
- [ ] `P1-NET-C`

## 2) Required test target list (from `CMake/Tests.cmake`)

These are the required Phase 1 RC gate test targets referenced by the acceptance traceability table and CI-gate language:

- [ ] `raid_state_test`
- [ ] `raid_sync_test`
- [ ] `raid_protocol_test`
- [ ] `raid_progression_test`
- [ ] `encounter_schema_test`
- [ ] `encounter_engine_phase_test`
- [ ] `rollback_state_test`
- [ ] `net_telemetry_trace_test`

Suggested command blueprint (local or CI):

```bash
ctest -R "(raid_state_test|raid_sync_test|raid_protocol_test|raid_progression_test|encounter_schema_test|encounter_engine_phase_test|rollback_state_test|net_telemetry_trace_test)"
```

## 3) Exit criteria (roadmap milestone table + cross-track)

### Track milestone completion

- [ ] **Raid Core**: all Raid Core acceptance checks automated; CI green on raid suite; no known reward-dup or lockout-severity bugs open.
- [ ] **Netcode Integrity**: all netcode invariants enforced with tests; rollback correction replay validated under stress; telemetry round-trip suite green.
- [ ] **Dev Experience**: contributor entry map merged; docs link to key tests; first-time contributor dry-run completed and documented.

### Cross-track exit criteria

- [ ] Owners assigned and acknowledged in planning sync.
- [ ] ETA confidence >= 80% with explicit risk notes.
- [ ] CI gate includes required raid + rollback test subsets.
- [ ] Phase 1 sign-off recorded in engineering notes.

## 4) Additive-only confirmation (no removals)

Release-candidate confirmation for this Phase 1 gate:

- [ ] **No removals, additive only**: all checklist and implementation changes are additive, with no destructive scope reduction.
- [ ] Any exception was explicitly approved and documented with rationale, impact, rollback plan, and owner sign-off.

## Evidence log

- Build/CI run link(s):
- Test report artifact(s):
- Sign-off notes link:
- Exception records (if any):
