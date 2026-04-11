# Revolutionary Networking + Graphics Blueprint

## Vision (Do not break existing gameplay)
- Keep the current gameplay/runtime paths intact while adding **parallel “next-gen” pipelines** behind feature flags.
- Ship in layers so nothing critical is removed until replacements are proven in production.

## North-Star Targets
- 10x lower network stutter under packet loss.
- Deterministic multiplayer simulation with rollback + prediction.
- 2x frame throughput on the same GPU tier via render-graph scheduling.
- Hot-swappable quality/perf profiles per platform.

---

## Delivery Status Semantics (Anti False-Positive Rules)
- **Scaffold Completed**: Interface/flag/plumbing exists and compiles, but behavior is not yet validated end-to-end.
- **Feature Operational**: Real behavior is live behind a feature flag, validated by automated tests + runtime checks.
- A milestone item may only be marked done when it reaches **Feature Operational**.
- All status lines below explicitly carry one of: `Scaffold Completed` or `Feature Operational`.

---

## Mega TODO Blueprint

## 1) Networking Pipeline Rework (NOVA-NET)

### 1.1 Transport Abstraction (Additive)
- [x] **Scaffold Completed**: Introduce a transport interface (`INetTransport`) with adapters for:
  - [ ] UDP (baseline)
  - [ ] QUIC (reliable streams + datagrams)
  - [x] **Feature Operational**: Local loopback simulation mode.
    - Acceptance Criterion: `Source/engine/net/transport/loopback_sim.*` handles send/recv with deterministic ordering under configured chaos seed; validated by `test/net/test_loopback_sim_determinism.cpp` (`LoopbackSim_DeterministicOrdering`).
  - Acceptance Criterion: `Source/engine/net/transport/i_net_transport.*` defines connect/send/recv/flush/close contract used by all adapters; validated by `test/net/test_transport_contract.cpp` (`TransportContract_AllAdaptersImplementCoreCalls`).
- [x] **Feature Operational**: Add runtime transport selection (`--net-transport=udp|quic|sim`).
  - Acceptance Criterion: `Source/diablo_startup.cpp` parses and applies CLI transport mode with explicit fallback to `udp`; validated by `test/app/test_net_transport_cli.cpp` (`NetTransportCli_ParsesAndAppliesMode`).

### 1.2 Tick-Accurate Deterministic Core
- [ ] Introduce fixed-step simulation boundary (`SimTick`) decoupled from render FPS.
- [ ] Build deterministic state hashing each N ticks for divergence detection.
- [ ] Add replay stream format with input-only logs + checksum timeline.

### 1.3 Rollback + Prediction Layer
- [ ] Add ring-buffer world snapshots (configurable depth).
- [ ] Predict local player actions immediately.
- [ ] On remote correction: rollback to divergence tick, re-simulate forward.
- [ ] Add reconciliation metrics HUD (prediction error, rollback count, max rewind).

### 1.4 Interest Management + Replication Graph
- [ ] Spatial channeling (grid/rooms) to avoid full-world replication.
- [ ] Priority lanes: critical (combat), important (AI), cosmetic (fx).
- [ ] Delta-compressed component replication with schema versioning.

### 1.5 Congestion + QoS Intelligence
- [x] **Feature Operational**: Add runtime chaos profile CLI controls for simulation transport (drop/dup/reorder/seed).
  - Acceptance Criterion: `Source/engine/net/chaos/chaos_profile_cli.*` maps CLI values to runtime profile and rejects invalid ranges; validated by `test/net/test_chaos_profile_cli.cpp` (`ChaosProfileCli_AppliesAndValidatesArgs`).
- [x] **Feature Operational**: Packet budget allocator per frame using moving RTT/loss/jitter windows.
  - Acceptance Criterion: `Source/engine/net/qos/packet_budget_allocator.*` enforces min/max packet budget envelope and converges within 60 frames after loss spike; validated by `test/net/test_packet_budget_allocator.cpp` (`PacketBudget_ConvergesAfterLossSpike`).
- [ ] Dynamic reliability policy (auto-upgrade event channels when loss spikes).
- [ ] FEC pilot mode for high-loss links.

### 1.6 Security + Integrity
- [ ] Message authentication tags on gameplay-critical packets.
- [ ] Anti-replay nonce windows.
- [ ] Optional encrypted session channels.

### 1.7 Observability First
- [ ] Always-on telemetry counters:
  - [ ] RTT, jitter, loss, resend %, divergence %, rollback ms
- [ ] Frame-accurate net trace export for offline analysis.
- [ ] In-game net debugger overlay.

---

## 2) Graphics Pipeline Rework (AURORA-GFX)

### 2.1 Render Graph Architecture (Additive)
- [x] **Feature Operational**: Add feature-flagged render-graph execution path in frame loop with legacy fallback.
  - Acceptance Criterion: `Source/engine/render/render_graph/frame_executor.*` toggles between render-graph and legacy path using `Gfx.RenderGraph` and auto-falls back on node compile failure; validated by `test/render/test_render_graph_toggle.cpp` (`RenderGraph_ToggleAndFallback`).
- [ ] Replace ad-hoc pass ordering with declarative render-graph DAG.
- [ ] Explicit resource lifetime + transient texture aliasing.
- [ ] Automatic barrier/sync generation per backend.

### 2.2 Multi-Backend Modernization
- [x] **Scaffold Completed**: Define backend-agnostic RHI interface (`IRenderBackend`).
  - Acceptance Criterion: `Source/engine/render/rhi/i_render_backend.*` exposes backend contract (device init, frame begin/end, resource create/destroy) with no backend-specific types in interface; validated by `test/render/test_rhi_interface_contract.cpp` (`RhiInterface_BackendAgnosticContract`).
- [ ] Keep current backend as compatibility path.
- [ ] Add staged support for modern API path (e.g., Vulkan/Metal/DX12-like model).

### 2.3 GPU-Driven Frame
- [ ] Move culling + instance compaction to GPU compute.
- [ ] Indirect draw buffers for large scene batches.
- [ ] Bindless-friendly descriptor model where supported.

### 2.4 Async Compute + Frame Pipelining
- [ ] Run post-processing/light prep on async compute queues.
- [ ] Double/triple-frame resource staging for CPU/GPU overlap.
- [ ] Introduce frame graph timing lanes (cpu main, copy, graphics, compute).

### 2.5 Scalable Effects Stack
- [ ] Temporal upscaling abstraction (FSR-like / TAAU style hook points).
- [ ] Dynamic resolution based on frametime budget.
- [ ] Tiered post-process chain by hardware class.

### 2.6 Shader System 2.0
- [ ] Unified shader metadata + permutation key system.
- [ ] Background shader compile cache + warmup packs.
- [ ] Cross-platform shader pipeline cache persistence.

### 2.7 Diagnostics + Tooling
- [ ] GPU markers on every render-graph node.
- [ ] Live VRAM budget HUD + overcommit warnings.
- [ ] Capture-friendly debug modes for deterministic frame repro.

---

## 3) Cross-Cutting Performance Engine
- [ ] Create a central frame budget manager (simulation/net/render).
- [ ] Per-subsystem quality governors driven by frametime + latency.
- [ ] Burst-safe job system lanes for net decode, animation, and render prep.

---

## 4) Compatibility & Safety Strategy (Do not remove first)
- [ ] Feature flags for every new subsystem:
  - [ ] `Net.NovaTransport`
  - [ ] `Net.Rollback`
  - [ ] `Gfx.RenderGraph`
  - [ ] `Gfx.GpuDriven`
- [ ] Side-by-side execution mode for A/B validation.
- [ ] Golden test scenes + deterministic multiplayer regression packs.
- [ ] Auto-fallback to legacy path on instability.

---

## 5) Milestone Plan

### Milestone Status Mapping Table

| Milestone Item | Current Classification | Code Location | Expected Test File | Feature Flag |
|---|---|---|---|---|
| A: Interfaces + flags + deterministic split + render graph skeleton | Scaffold Completed | `Source/engine/net/transport/`, `Source/engine/render/rhi/`, `Source/engine/render/render_graph/` | `test/net/test_transport_contract.cpp`, `test/render/test_rhi_interface_contract.cpp`, `test/render/test_render_graph_toggle.cpp` | `Net.NovaTransport`, `Gfx.RenderGraph` |
| B: Rollback MVP + replication graph MVP + basic render graph passes | Not Started | `Source/engine/net/rollback/`, `Source/engine/net/replication/`, `Source/engine/render/render_graph/passes/` | `test/net/test_rollback_mvp.cpp`, `test/net/test_replication_graph_mvp.cpp`, `test/render/test_render_graph_basic_passes.cpp` | `Net.Rollback`, `Net.NovaTransport`, `Gfx.RenderGraph` |
| C: GPU-driven draws, async compute, QUIC adapter, dynamic budgets | Not Started | `Source/engine/render/gpu_driven/`, `Source/engine/render/async_compute/`, `Source/engine/net/transport/quic/`, `Source/engine/net/qos/` | `test/render/test_gpu_driven_draws.cpp`, `test/render/test_async_compute_pipeline.cpp`, `test/net/test_quic_transport.cpp`, `test/net/test_packet_budget_allocator.cpp` | `Gfx.GpuDriven`, `Gfx.AsyncCompute`, `Net.NovaTransport` |
| D: Telemetry, replay validation, perf bake-offs, fallback tuning | Not Started | `Source/engine/net/telemetry/`, `Source/engine/net/replay/`, `Source/engine/perf/`, `Source/engine/fallback/` | `test/net/test_telemetry_counters.cpp`, `test/net/test_replay_validation.cpp`, `test/perf/test_bakeoff_baselines.cpp`, `test/core/test_auto_fallback.cpp` | `Net.Telemetry`, `Net.Replay`, `Core.AutoFallback` |
| E: Default-on on supported platforms, legacy retained as fallback | Not Started | `Source/engine/config/feature_flags.*`, `Source/engine/platform/` | `test/config/test_default_on_matrix.cpp`, `test/core/test_legacy_fallback_guard.cpp` | `Net.NovaTransport`, `Net.Rollback`, `Gfx.RenderGraph`, `Gfx.GpuDriven` |

### Milestone A (Foundation)
- [ ] Interfaces, flags, deterministic tick split, render graph skeleton.

**Definition of Done (A)**
- `INetTransport` + `IRenderBackend` compile on all tier-1 targets with no platform-specific leakage in interfaces.
- Deterministic tick split is executable behind flag and validated by repeatable checksum test.
- Render-graph skeleton can execute world/UI/post path with parity baseline image checks.
- Compatibility/Fallback: legacy net + legacy render remain default; new paths require explicit enable via feature flags.

### Milestone B (Prototype)
- [ ] Rollback MVP + replication graph MVP + basic render graph passes.

**Definition of Done (B)**
- Rollback MVP rewinds/re-simulates correctly in deterministic integration test.
- Replication graph MVP reduces payload vs full-world baseline by agreed threshold in synthetic scene.
- Basic render graph pass chain produces expected frame output on reference map.
- Compatibility/Fallback: if rollback divergence exceeds threshold or pass build fails, runtime auto-reverts to non-rollback or legacy pass chain.

### Milestone C (Acceleration)
- [ ] GPU-driven draws, async compute, QUIC adapter, dynamic budgets.

**Definition of Done (C)**
- GPU-driven pipeline improves draw submission cost in benchmark scene vs CPU path.
- Async compute lanes show measurable overlap in timing capture.
- QUIC adapter passes reliability/interruption recovery tests.
- Dynamic packet budgets remain within latency SLA under chaos profiles.
- Compatibility/Fallback: disable GPU-driven/async/QUIC independently at runtime; hard fallback to stable backend if capability probe fails.

### Milestone D (Hardening)
- [ ] Telemetry, replay validation, perf bake-offs, fallback tuning.

**Definition of Done (D)**
- Telemetry counters exported continuously with frame index correlation.
- Replay validation detects divergence and emits actionable diagnostics.
- Perf bake-off suite runs in CI and enforces regression budget.
- Compatibility/Fallback: fallback triggers are rate-limited, observable, and reversible without restart where feasible.

### Milestone E (Production)
- [ ] Default-on for supported platforms, legacy retained as fallback.

**Definition of Done (E)**
- Feature flags default to ON only for validated platform matrix.
- Crash-free/session-stability and performance SLOs met for release window.
- Rollout includes staged canary + automated rollback policy.
- Compatibility/Fallback: legacy transport/render paths are still shippable, test-covered, and can be forced by CLI/config on every supported platform.

---

## 6) Immediate Next Actions (Highest ROI)
- [x] **Scaffold Completed**: Draft `INetTransport` and `IRenderBackend` interface headers.
  - Acceptance Criterion: `Source/engine/net/transport/i_net_transport.*` and `Source/engine/render/rhi/i_render_backend.*` compile cleanly with unit contract tests; validated by `test/net/test_transport_contract.cpp` and `test/render/test_rhi_interface_contract.cpp`.
- [x] **Feature Operational**: Add feature-flag config plumbing and startup parsing.
  - Acceptance Criterion: `Source/engine/config/feature_flags.*` and startup parsing apply flags at boot and persist per-session overrides; validated by `test/config/test_feature_flag_startup.cpp` (`FeatureFlags_StartupPlumbing`).
- [x] **Scaffold Completed**: Add deterministic tick boundary + hash instrumentation.
  - Acceptance Criterion: `Source/engine/sim/sim_tick.*` and `Source/engine/sim/state_hash.*` expose tick/hash APIs and emit hash samples every configured N ticks; validated by `test/sim/test_sim_tick_hash.cpp` (`SimTickHash_EmitsConfiguredCadence`).
- [x] **Feature Operational**: Implement minimal render graph with 3 passes (world, UI, post).
  - Acceptance Criterion: `Source/engine/render/render_graph/passes/{world,ui,post}.*` run in deterministic order and complete frame without legacy path; validated by `test/render/test_render_graph_three_pass.cpp` (`RenderGraph_ThreePassExecution`).
- [ ] Create benchmark map + scripted network chaos test.
  - [x] **Feature Operational**: Scripted network chaos injector harness (drop/duplicate/reorder profile core).
    - Acceptance Criterion: `test/harness/net/chaos_injector_harness.*` can replay seed and reproduce packet mutation sequence exactly across runs; validated by `test/net/test_chaos_harness_replay.cpp` (`ChaosHarness_ReplayDeterministic`).
  - [x] **Scaffold Completed**: Chaos processing micro-benchmark harness.
    - Acceptance Criterion: `bench/net/chaos_processing_bench.cpp` outputs stable p50/p95 metrics format consumable by CI artifact parser; validated by `test/bench/test_chaos_bench_output.cpp` (`ChaosBench_OutputSchema`).

## Approval Gates Requested
1. Approve blueprint scope and naming (`NOVA-NET`, `AURORA-GFX`).
2. Choose first implementation stream:
   - Networking-first
   - Graphics-first
   - Dual-track (slower but parallel)
3. Set target platform priority (PC first vs all platforms parity).
