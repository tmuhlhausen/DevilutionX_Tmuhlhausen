#include "guild/guild_progression.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace devilution {
namespace {

GuildProgressionPersistedState ProgressionState {};
std::vector<GrantedGuildReward> GrantedRewards {};

constexpr size_t ActivityIndex(GuildActivityType activity)
{
	return static_cast<size_t>(activity);
}

uint64_t HashGuildRewardKey(uint32_t guildId, uint32_t raidId, GuildActivityType activity, uint16_t milestone, std::string_view rewardId)
{
	uint64_t hash = 1469598103934665603ULL;
	auto hashByte = [&](uint8_t value) {
		hash ^= value;
		hash *= 1099511628211ULL;
	};
	auto hashU32 = [&](uint32_t value) {
		for (size_t i = 0; i < sizeof(value); i++)
			hashByte(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
	};

	hashU32(guildId);
	hashU32(raidId);
	hashByte(static_cast<uint8_t>(activity));
	hashByte(static_cast<uint8_t>(milestone & 0xFF));
	hashByte(static_cast<uint8_t>((milestone >> 8) & 0xFF));
	for (const char value : rewardId)
		hashByte(static_cast<uint8_t>(value));
	return hash;
}

bool HasDedupKey(uint64_t dedupKey)
{
	const size_t count = std::min(static_cast<size_t>(ProgressionState.usedDedupKeys), ProgressionState.dedupKeys.size());
	for (size_t i = 0; i < count; i++) {
		if (ProgressionState.dedupKeys[i] == dedupKey)
			return true;
	}
	return false;
}

void AddDedupKey(uint64_t dedupKey)
{
	if (HasDedupKey(dedupKey))
		return;

	if (ProgressionState.usedDedupKeys < ProgressionState.dedupKeys.size()) {
		ProgressionState.dedupKeys[ProgressionState.usedDedupKeys++] = dedupKey;
		return;
	}

	std::rotate(ProgressionState.dedupKeys.begin(), ProgressionState.dedupKeys.begin() + 1, ProgressionState.dedupKeys.end());
	ProgressionState.dedupKeys.back() = dedupKey;
}

void GrantDeterministicReward(const GuildRewardDefinition &definition, uint32_t raidId)
{
	const uint64_t dedupKey = HashGuildRewardKey(ProgressionState.guildId.value, raidId, definition.activity, definition.milestone, definition.rewardId);
	if (HasDedupKey(dedupKey))
		return;

	AddDedupKey(dedupKey);
	GrantedRewards.push_back(GrantedGuildReward {
	    dedupKey,
	    definition.activity,
	    definition.milestone,
	    definition.rewardQuantity,
	});
}

void ResolveRewardsForActivity(GuildActivityType activity, uint8_t guildLevel, uint32_t raidId, uint32_t previousValue, uint32_t newValue)
{
	const size_t index = ActivityIndex(activity);
	for (const GuildRewardDefinition &definition : GuildRewardDefinitions) {
		if (definition.activity != activity || guildLevel < definition.minGuildLevel)
			continue;
		if (definition.milestone == 0 || definition.milestone > MaxGuildProgressionMilestones)
			continue;
		if (definition.milestone <= previousValue || definition.milestone > newValue)
			continue;

		ProgressionState.completedMilestones[index] |= (UINT64_C(1) << (definition.milestone - 1));
		GrantDeterministicReward(definition, raidId);
	}
}

} // namespace

void ResetGuildProgression()
{
	ProgressionState = {};
	GrantedRewards.clear();
}

GuildProgressionPersistedState GetGuildProgressionPersistedState()
{
	return ProgressionState;
}

void ApplyGuildProgressionPersistedState(const GuildProgressionPersistedState &state)
{
	ProgressionState = state;
	ProgressionState.usedDedupKeys = std::min<uint16_t>(ProgressionState.usedDedupKeys, ProgressionState.dedupKeys.size());
}

void EmitGuildActivityEvent(GuildId guildId, GuildActivityType activity, uint16_t amount, uint8_t guildLevel, uint32_t raidId)
{
	if (!guildId.IsValid() || amount == 0)
		return;

	if (ProgressionState.guildId != guildId)
		ResetGuildProgression();
	ProgressionState.guildId = guildId;

	const size_t index = ActivityIndex(activity);
	const uint32_t previousValue = ProgressionState.counters[index];
	const uint32_t nextValue = std::min<uint32_t>(UINT32_MAX, previousValue + amount);
	ProgressionState.counters[index] = nextValue;
	ResolveRewardsForActivity(activity, guildLevel, raidId, previousValue, nextValue);
}

void HandleRaidCompletionForGuild(uint32_t raidId, GuildId guildId, uint8_t guildLevel, uint8_t bossKillCount)
{
	if (!guildId.IsValid())
		return;
	EmitGuildActivityEvent(guildId, GuildActivityType::DungeonClear, 1, guildLevel, raidId);
	if (bossKillCount != 0)
		EmitGuildActivityEvent(guildId, GuildActivityType::BossKill, bossKillCount, guildLevel, raidId);
}

const std::vector<GrantedGuildReward> &GetGrantedGuildRewards()
{
	return GrantedRewards;
}

} // namespace devilution
