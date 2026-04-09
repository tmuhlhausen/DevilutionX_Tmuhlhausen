# Progression & Loot Tuning Targets

## Target Time-to-500

These targets assume efficient multiplayer dungeon clearing with limited downtime.

- **Normal (1-200):** 10-14 hours
- **Nightmare (201-350):** 14-20 hours
- **Hell (351-500):** 20-30 hours
- **Total target to level 500:** **44-64 hours**

## Expected Loot per Hour

Ranges assume full-clear style play and include gold + equipment drops.

- **Normal:** 90-130 total drops/hour, 1-3 high-quality items/hour
- **Nightmare:** 110-150 total drops/hour, 3-6 high-quality items/hour
- **Hell:** 120-170 total drops/hour, 5-10 high-quality items/hour

## Curve Inputs

Progression and loot curves are configured via `assets/txtdata/progression_curves.tsv`:

- `XpGainPercent` (hard-clamped to **100-500**) scales post-delta XP.
- `*DropRatePermille` scales weighted drop-rate rolls by difficulty.
- `*AncientChancePermille` scales Ancient rarity weight by difficulty.

## Multiplayer Integrity Hooks

The progression integrity layer emits warning logs when:

- XP jumps are unusually large in multiplayer sessions.
- Remote Nephilim packet progression regresses or exceeds bounded deltas.
- Incoming item packets contain invalid bounded payload values (durability/charges/id bits).

Use these warnings as telemetry for anti-cheat triage and tuning regressions.
