#include "dvlnet/rollback_state.hpp"

#include <algorithm>

namespace devilution::dvlnet {

RollbackState::RollbackState(size_t depth)
    : depth_(std::max<size_t>(1, depth))
    , ring_(depth_)
{
}

void RollbackState::Reset()
{
	for (Slot &slot : ring_) {
		slot.meta = {};
		slot.snapshot.clear();
	}
}

size_t RollbackState::SlotIndex(uint64_t tick) const
{
	return static_cast<size_t>(tick % depth_);
}

const RollbackState::Slot *RollbackState::FindSlot(uint64_t tick) const
{
	const Slot &slot = ring_[SlotIndex(tick)];
	if (slot.meta.tick != tick)
		return nullptr;
	return &slot;
}

RollbackState::Slot *RollbackState::FindSlot(uint64_t tick)
{
	Slot &slot = ring_[SlotIndex(tick)];
	if (slot.meta.tick != tick)
		return nullptr;
	return &slot;
}

void RollbackState::StoreSnapshot(uint64_t tick, std::span<const std::byte> worldSnapshot, uint32_t stateHash)
{
	Slot &slot = ring_[SlotIndex(tick)];
	slot.meta.tick = tick;
	slot.meta.stateHash = stateHash;
	slot.meta.hasSnapshot = true;
	slot.snapshot.assign(worldSnapshot.begin(), worldSnapshot.end());
}

void RollbackState::QueuePredictedInput(uint64_t tick, std::span<const std::byte> input)
{
	Slot &slot = ring_[SlotIndex(tick)];
	if (slot.meta.tick != tick) {
		slot.meta = {};
		slot.meta.tick = tick;
	}
	slot.meta.input.assign(input.begin(), input.end());
}

std::optional<uint32_t> RollbackState::GetHash(uint64_t tick) const
{
	const Slot *slot = FindSlot(tick);
	if (slot == nullptr)
		return std::nullopt;
	return slot->meta.stateHash;
}

bool RollbackState::HasSnapshot(uint64_t tick) const
{
	const Slot *slot = FindSlot(tick);
	return slot != nullptr && slot->meta.hasSnapshot;
}

bool RollbackState::DetectDivergence(uint64_t tick, uint32_t authoritativeHash) const
{
	const std::optional<uint32_t> localHash = GetHash(tick);
	if (!localHash.has_value())
		return false;
	return localHash.value() != authoritativeHash;
}

bool RollbackState::HandleCorrection(uint64_t authoritativeTick, uint32_t authoritativeHash, uint64_t currentTick,
    const std::function<bool(std::span<const std::byte>)> &restoreSnapshot,
    const std::function<void(std::span<const std::byte>)> &replayInput) const
{
	const Slot *authoritativeSlot = FindSlot(authoritativeTick);
	if (authoritativeSlot == nullptr || !authoritativeSlot->meta.hasSnapshot)
		return false;
	if (!DetectDivergence(authoritativeTick, authoritativeHash))
		return false;
	if (!restoreSnapshot(authoritativeSlot->snapshot))
		return false;

	for (uint64_t tick = authoritativeTick + 1; tick <= currentTick; ++tick) {
		const Slot *slot = FindSlot(tick);
		if (slot == nullptr)
			continue;
		replayInput(slot->meta.input);
	}
	return true;
}

RollbackState &GetRollbackState()
{
	static RollbackState Rollback;
	return Rollback;
}

void ResetRollbackState()
{
	GetRollbackState().Reset();
}

} // namespace devilution::dvlnet
