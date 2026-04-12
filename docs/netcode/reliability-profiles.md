# Reliability Profiles Tuning Playbook

This playbook describes how to tune rollback reliability in three policy profiles:

- **Aggressive**: highest correction depth and resend budget for stable/low-latency links.
- **Balanced**: default profile for mixed network conditions.
- **Conservative**: lower correction and resend limits to prevent stalls on unstable links.

## Profile Baselines

| Profile | Max correction depth | Max resends in-flight | RTT gate (ms) | Drop gate (%) |
|---|---:|---:|---:|---:|
| Aggressive | 24 | 4 | 300 | 10 |
| Balanced | 12 | 2 | 225 | 4 |
| Conservative | 6 | 1 | 160 | 2 |

## Telemetry Signals Used

Adaptive logic reads rolling net telemetry:

- RTT / jitter / drop / resend percentages.
- Percentile windows: RTT p50, RTT p95, jitter p95.
- Anomaly markers:
  - `anom_latency`: current RTT exceeds p95 envelope.
  - `anom_drop`: drop bursts over rolling baseline.
  - `anom_div`: divergence spikes over baseline.

## Adaptive Threshold Rules

1. Start from current profile baseline.
2. If RTT or drop exceeds profile gates:
   - halve correction depth (minimum 3),
   - reduce resend allowance by 1 (minimum 1).
3. If resend pressure exceeds 12%:
   - force resend allowance to 1.
4. Gameplay loop profile selection:
   - **Conservative** when latency/drop anomalies are active,
   - **Aggressive** when RTT, drop, and resend are all low,
   - otherwise **Balanced**.

## Validation Workflow

1. Enable net telemetry trace (`jsonl` or `tsv`).
2. Run deterministic net-sim scenarios:
   - latency+jitter stability,
   - drop density bounds,
   - reorder detection assertions.
3. Verify sync assertions remain deterministic across seeds.
4. Promote profile updates only when divergence spikes regress to baseline.

## Recommended Iteration Cadence

- Tune profile constants weekly from trace samples.
- Re-run deterministic net-sim tests on every rollback/transport change.
- Prefer small profile adjustments (1 variable at a time) to preserve determinism.
