# Encounter Schema (JSON/TOML)

This schema defines raid encounter scripts consumed by `Source/raid/encounter/encounter_schema.*`.

## Top-level fields
- `encounter_id` (`u32`, required)
- `start_phase_id` (`u8`, required)
- `enrage_time_ms` (`u32`, optional, default `0`)
- `phases` (`array`, required)
- `mechanics` (`array`, optional)
- `rewards` (`array`, optional)

## Phase object
- `id` (`u8`, required, unique)
- `failure_policy` (`Wipe | PartialFail | CheckpointRollback`, required)
- `exit_condition` (`<ConditionType>:<threshold>`, optional)
- `next_phase_id` (`u8`, optional, must reference a valid phase)

### Scripted transitions
If `next_phase_id` is set and `exit_condition` evaluates true, runtime transitions to that phase.
If omitted, runtime falls back to ascending phase-id progression.

## Mechanic object
- `id` (`u16`, required, unique)
- `type` (`AoEPulse | AddWave | PositionalCheck | Enrage`, required)
- `phase_id` (`u8`, required)
- `period_ms` (`u32`, optional)
- `first_trigger_ms` (`u32`, optional)
- `repeat` (`bool`, optional, default `true`)
- `trigger_condition` (`<ConditionType>:<threshold>`, optional)

Mechanics execute only in the active phase and only when `trigger_condition` (if present) evaluates true.

## Condition types
- `None`
- `TimeElapsedAtLeast`
- `BossHealthAtMost`
- `AddsDefeatedAtLeast`
- `PlayerDeathsAtLeast`
- `AllPlayersDead`

## Reward object
- `id` (`string`, required, non-empty)
- `quantity` (`u32`, required, > 0)

## TOML example
```toml
encounter_id = 9001
start_phase_id = 0
enrage_time_ms = 300000

[[phases]]
id = 0
failure_policy = "Wipe"
exit_condition = "TimeElapsedAtLeast:45000"
next_phase_id = 2

[[phases]]
id = 2
failure_policy = "CheckpointRollback"
exit_condition = "BossHealthAtMost:0"

[[mechanics]]
id = 100
type = "AoEPulse"
phase_id = 0
period_ms = 10000
first_trigger_ms = 5000
repeat = true
trigger_condition = "BossHealthAtMost:85"

[[rewards]]
id = "raid-token"
quantity = 5
```

## JSON example
```json
{
  "encounter_id": 9001,
  "start_phase_id": 0,
  "enrage_time_ms": 300000,
  "phases": [
    { "id": 0, "failure_policy": "Wipe", "exit_condition": "TimeElapsedAtLeast:45000", "next_phase_id": 2 },
    { "id": 2, "failure_policy": "CheckpointRollback", "exit_condition": "BossHealthAtMost:0" }
  ],
  "mechanics": [
    { "id": 100, "type": "AoEPulse", "phase_id": 0, "period_ms": 10000, "first_trigger_ms": 5000, "repeat": true, "trigger_condition": "BossHealthAtMost:85" }
  ],
  "rewards": [
    { "id": "raid-token", "quantity": 5 }
  ]
}
```
