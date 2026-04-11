#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace devilution {

constexpr uint8_t RaidEncounterBuckets = 16;
constexpr uint8_t MaxMultiplayerLevelValue = std::numeric_limits<uint8_t>::max();

[[nodiscard]] constexpr uint8_t GetMaxActiveRaidInstances(uint8_t raidMultiplayerBase)
{
	return raidMultiplayerBase > MaxMultiplayerLevelValue
	    ? 0
	    : static_cast<uint8_t>((MaxMultiplayerLevelValue - raidMultiplayerBase + 1) / RaidEncounterBuckets);
}

[[nodiscard]] constexpr size_t GetRaidMultiplayerLevelSpan(uint8_t raidMultiplayerBase)
{
	return static_cast<size_t>(GetMaxActiveRaidInstances(raidMultiplayerBase)) * RaidEncounterBuckets;
}

enum class RaidRoutingDenialReason : uint8_t {
	None,
	InvalidRaidId,
	InstanceCapacityExceeded,
	BaseOutOfRange,
};

struct RaidRoutingResult {
	uint8_t level = MaxMultiplayerLevelValue;
	RaidRoutingDenialReason denialReason = RaidRoutingDenialReason::None;

	[[nodiscard]] bool IsDenied() const
	{
		return denialReason != RaidRoutingDenialReason::None;
	}
};

[[nodiscard]] uint64_t ComputeRaidMultiplayerKey(uint32_t guildId, uint32_t raidId, uint8_t encounterIndex, uint32_t shard);
[[nodiscard]] RaidRoutingResult ComputeRaidMultiplayerLevel(uint32_t guildId, uint32_t raidId, uint8_t encounterIndex, uint32_t shard, uint8_t raidMultiplayerBase);

} // namespace devilution
