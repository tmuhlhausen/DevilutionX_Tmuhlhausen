#include "raid/raid_routing.hpp"

#include <algorithm>

namespace devilution {

namespace {

constexpr uint64_t FnvOffsetBasis = 1469598103934665603ULL;
constexpr uint64_t FnvPrime = 1099511628211ULL;

void MixIntoHash(uint64_t &hash, uint64_t value)
{
	for (int shift = 0; shift < 64; shift += 8) {
		hash ^= (value >> shift) & 0xFF;
		hash *= FnvPrime;
	}
}

} // namespace

uint64_t ComputeRaidMultiplayerKey(uint32_t guildId, uint32_t raidId, uint8_t encounterIndex, uint32_t shard)
{
	uint64_t hash = FnvOffsetBasis;
	MixIntoHash(hash, guildId);
	MixIntoHash(hash, raidId);
	MixIntoHash(hash, encounterIndex);
	MixIntoHash(hash, shard);
	return hash;
}

RaidRoutingResult ComputeRaidMultiplayerLevel(uint32_t guildId, uint32_t raidId, uint8_t encounterIndex, uint32_t shard, uint8_t raidMultiplayerBase)
{
	if (raidId == 0)
		return { MaxMultiplayerLevelValue, RaidRoutingDenialReason::InvalidRaidId };

	const uint8_t maxActiveRaidInstances = GetMaxActiveRaidInstances(raidMultiplayerBase);
	if (maxActiveRaidInstances == 0)
		return { MaxMultiplayerLevelValue, RaidRoutingDenialReason::BaseOutOfRange };
	if (shard >= maxActiveRaidInstances)
		return { MaxMultiplayerLevelValue, RaidRoutingDenialReason::InstanceCapacityExceeded };

	const uint8_t normalizedEncounter = encounterIndex % RaidEncounterBuckets;
	const uint8_t raidBucket = static_cast<uint8_t>(ComputeRaidMultiplayerKey(guildId, raidId, 0, shard) % maxActiveRaidInstances);
	const uint16_t offset = static_cast<uint16_t>(raidBucket) * RaidEncounterBuckets + normalizedEncounter;
	const uint16_t level = raidMultiplayerBase + offset;
	const uint8_t routedLevel = std::min<uint16_t>(level, MaxMultiplayerLevelValue);
	return { routedLevel, RaidRoutingDenialReason::None };
}

} // namespace devilution
