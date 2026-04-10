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

## Mega TODO Blueprint

## 1) Networking Pipeline Rework (NOVA-NET)

### 1.1 Transport Abstraction (Additive)
- [x] Introduce a transport interface (`INetTransport`) with adapters for:
  - [ ] UDP (baseline)
  - [ ] QUIC (reliable streams + datagrams)
  - [x] Local loopback simulation mode
- [x] Add runtime transport selection (`--net-transport=udp|quic|sim`).

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
- [ ] Packet budget allocator per frame using moving RTT/loss/jitter windows.
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
- [ ] Replace ad-hoc pass ordering with declarative render-graph DAG.
- [ ] Explicit resource lifetime + transient texture aliasing.
- [ ] Automatic barrier/sync generation per backend.

### 2.2 Multi-Backend Modernization
- [x] Define backend-agnostic RHI interface (`IRenderBackend`).
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

### Milestone A (Foundation)
- [ ] Interfaces, flags, deterministic tick split, render graph skeleton.

### Milestone B (Prototype)
- [ ] Rollback MVP + replication graph MVP + basic render graph passes.

### Milestone C (Acceleration)
- [ ] GPU-driven draws, async compute, QUIC adapter, dynamic budgets.

### Milestone D (Hardening)
- [ ] Telemetry, replay validation, perf bake-offs, fallback tuning.

### Milestone E (Production)
- [ ] Default-on for supported platforms, legacy retained as fallback.

---

## 6) Immediate Next Actions (Highest ROI)
- [x] Draft `INetTransport` and `IRenderBackend` interface headers.
- [x] Add feature-flag config plumbing and startup parsing.
- [x] Add deterministic tick boundary + hash instrumentation.
- [x] Implement minimal render graph with 3 passes (world, UI, post).
- [ ] Create benchmark map + scripted network chaos test.
  - [x] Scripted network chaos injector harness (drop/duplicate/reorder profile core).
  - [x] Chaos processing micro-benchmark harness.

## Approval Gates Requested
1. Approve blueprint scope and naming (`NOVA-NET`, `AURORA-GFX`).
2. Choose first implementation stream:
   - Networking-first
   - Graphics-first
   - Dual-track (slower but parallel)
3. Set target platform priority (PC first vs all platforms parity).
