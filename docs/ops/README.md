# Observability & Ops

## Scope and goals

This document defines operational readiness for multiplayer reliability: structured logging, dashboards, diagnostics bundles, and incident response procedures.

Goals:
- Standardize high-signal logs across critical gameplay/network paths.
- Tie service-level indicators to release gates.
- Reduce triage time through on-demand diagnostics bundles.
- Establish a repeatable incident response and escalation process.

## Acceptance criteria mapped from README.md roadmap bullets

| Roadmap bullet (README.md) | Acceptance criteria | Evidence |
|---|---|---|
| Instrument structured gameplay/network logs with trace IDs, achieving `>= 95%` coverage on critical multiplayer paths. | Trace IDs are propagated end-to-end; log schema validated in CI; coverage report shows ≥95% critical path instrumentation. | Logging schema tests + coverage artifact. |
| Build service-level dashboards (latency, disconnects, desyncs, crash-free sessions) with alert thresholds tied to release gates. | Dashboard panels for all listed SLOs exist; threshold values are codified; release workflow blocks when gate thresholds fail. | Dashboard config + release gate check output. |
| Add on-demand diagnostics bundles that capture logs/config/runtime metadata and cut triage-to-root-cause time by `50%`. | Bundle command captures required artifacts with redaction policy; quarterly benchmark indicates ≥50% triage-time reduction versus baseline. | Diagnostics bundle smoke tests + benchmark report. |
| Document incident response runbooks and on-call escalation paths with quarterly drill completion tracked in `docs/ops/`. | Runbook includes severity model, escalation paths, and postmortem template; quarterly drill checklist is versioned and updated. | Drill records and runbook revision history in `docs/ops/`. |

## Required tests and file ownership

Required tests:
- Log schema and trace propagation validation tests.
- Dashboard threshold contract tests.
- Diagnostics bundle smoke/integration tests.
- Incident drill checklist validation for required fields.

Suggested ownership:
- Primary: Platform/Ops maintainers.
- Secondary: Multiplayer engineering for instrumentation.
- Docs owner: `docs/ops/` maintainers for runbooks, drills, and SLO definitions.

## Status

| Capability | Scaffold Completed | Feature Operational | Notes |
|---|---|---|---|
| Structured logs with trace IDs | ✅ | ⛔ | Scope and criteria documented; instrumentation rollout pending. |
| SLO dashboards and release gates | ✅ | ⛔ | Metrics and gates specified; dashboards/checks pending. |
| Diagnostics bundles | ✅ | ⛔ | Artifact contract documented; implementation pending. |
| Incident response runbooks & drills | ✅ | ⛔ | Runbook structure scaffolded; drill execution cadence pending. |
