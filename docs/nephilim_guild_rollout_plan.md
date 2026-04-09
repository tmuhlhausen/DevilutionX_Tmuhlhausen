# Nephilim + Guild Feature Rollout Plan

This rollout keeps all new systems **opt-in** until deterministic multiplayer validation and backward compatibility checks pass.

## Global release gates (applies to every phase)

1. Gate the phase behind a dedicated gameplay config flag.
2. Keep existing save/network compatibility tests green.
3. Require deterministic multiplayer item recreation checks before any new item-affecting phase is enabled by default.

## Phase A — Nephilim core + save/net compatibility + UI

- Implement Nephilim core domain model behind `Phase A - Nephilim Core`.
- Preserve legacy save format and existing network packet behavior when flag is off.
- Add UI surface only when phase flag is enabled.

## Phase B — New rarity system + drop pipeline + scaling

- Implement rarity tiers and drop pipeline adapters behind `Phase B - Rarity + Drops`.
- Apply scaling rules through a compatibility adapter so legacy item generation still works with flag off.
- Block default enablement until deterministic multiplayer item recreation checks pass.

## Phase C — Guild core protocol + membership persistence

- Add guild protocol message family behind `Phase C - Guild Core Protocol`.
- Persist guild membership data with explicit fallback for old saves.
- Keep old multiplayer handshake and packet flow untouched when phase flag is off.

## Phase D — Guild halls/maps + endgame loop integration

- Enable guild halls/maps content under `Phase D - Guild Halls + Endgame`.
- Integrate hall routing and endgame loop hooks through feature-gated map loaders.
- Ensure endgame loop additions are deterministic in multiplayer simulation.

## Phase E — balancing pass + anti-cheat hardening

- Apply post-feature balancing values behind `Phase E - Balance + Anti-cheat`.
- Add anti-cheat hardening validators around new item/guild state transitions.
- Keep compatibility behavior path available for rollback and A/B verification.
