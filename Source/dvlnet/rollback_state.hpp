#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace devilution::dvlnet {

struct RollbackTickMetadata {
	uint64_t tick = 0;
	uint32_t stateHash = 0;
	std::vector<std::byte> input;
	bool hasHash = false;
	bool hasSnapshot = false;
};

enum class RollbackReplayPolicyProfile {
	Aggressive,
	Balanced,
	Conservative,
};

struct RollbackReplayPolicy {
	uint32_t maxCorrectionDepth = 12;
	uint32_t maxResendsInFlight = 2;
	float divergenceRttGateMs = 225.0F;
	float dropGatePct = 4.0F;
};

struct RollbackAdaptiveThresholds {
	uint32_t maxCorrectionDepth = 12;
	uint32_t maxResendsInFlight = 2;
};

class RollbackState {
public:
	explicit RollbackState(size_t depth = 64);

	void Reset();
	void StoreSnapshot(uint64_t tick, std::span<const std::byte> worldSnapshot, uint32_t stateHash);
	void QueuePredictedInput(uint64_t tick, std::span<const std::byte> input);
	void SubmitAuthoritativeState(uint64_t tick, uint32_t stateHash);
	[[nodiscard]] std::optional<RollbackTickMetadata> ConsumeAuthoritativeState();
	[[nodiscard]] std::optional<uint32_t> GetHash(uint64_t tick) const;
	[[nodiscard]] bool HasSnapshot(uint64_t tick) const;
	[[nodiscard]] bool DetectDivergence(uint64_t tick, uint32_t authoritativeHash) const;
	void SetReplayPolicyProfile(RollbackReplayPolicyProfile profile);
	[[nodiscard]] RollbackReplayPolicyProfile ReplayPolicyProfile() const;
	[[nodiscard]] RollbackReplayPolicy ReplayPolicy() const;
	[[nodiscard]] RollbackAdaptiveThresholds ComputeAdaptiveThresholds(float rollingRttMs, float rollingDropPct, float resendPct) const;
	[[nodiscard]] uint32_t MaxResendsInFlight(float rollingRttMs, float rollingDropPct, float resendPct) const;

	bool HandleCorrection(uint64_t authoritativeTick, uint32_t authoritativeHash, uint64_t currentTick,
	    const std::function<bool(std::span<const std::byte>)> &restoreSnapshot,
	    const std::function<void(std::span<const std::byte>)> &replayInput) const;

private:
	struct Slot {
		RollbackTickMetadata meta;
		std::vector<std::byte> snapshot;
	};

	[[nodiscard]] size_t SlotIndex(uint64_t tick) const;
	[[nodiscard]] const Slot *FindSlot(uint64_t tick) const;
	Slot *FindSlot(uint64_t tick);

	size_t depth_;
	std::vector<Slot> ring_;
	std::optional<RollbackTickMetadata> pendingAuthoritativeState_;
	RollbackReplayPolicyProfile profile_ = RollbackReplayPolicyProfile::Balanced;
};

RollbackState &GetRollbackState();
void ResetRollbackState();

} // namespace devilution::dvlnet
