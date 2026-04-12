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

uint32_t CalculateAccumulatedSeasonActivity()
{
	uint64_t total = 0;
	for (size_t i = 0; i < GuildActivityTypeCount; i++) {
		const uint32_t current = ProgressionState.counters[i];
		const uint32_t baseline = ProgressionState.seasonCounterBaseline[i];
		total += current >= baseline ? current - baseline : 0;
	}
	return static_cast<uint32_t>(std::min<uint64_t>(total, UINT32_MAX));
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

bool HasClaimDedupKey(uint64_t dedupKey)
{
	const size_t count = std::min(static_cast<size_t>(ProgressionState.usedClaimDedupKeys), ProgressionState.claimDedupKeys.size());
	for (size_t i = 0; i < count; i++) {
		if (ProgressionState.claimDedupKeys[i] == dedupKey)
			return true;
	}
	return false;
}

void AddClaimDedupKey(uint64_t dedupKey)
{
	if (HasClaimDedupKey(dedupKey))
		return;

	if (ProgressionState.usedClaimDedupKeys < ProgressionState.claimDedupKeys.size()) {
		ProgressionState.claimDedupKeys[ProgressionState.usedClaimDedupKeys++] = dedupKey;
		return;
	}

	std::rotate(ProgressionState.claimDedupKeys.begin(), ProgressionState.claimDedupKeys.begin() + 1, ProgressionState.claimDedupKeys.end());
	ProgressionState.claimDedupKeys.back() = dedupKey;
}

void GrantDeterministicReward(const GuildRewardDefinition &definition, uint32_t raidId)
{
	const uint64_t dedupKey = HashGuildRewardKey(ProgressionState.guildId.value, raidId, definition.activity, definition.milestone, definition.rewardId);
	if (HasDedupKey(dedupKey))
		return;

	const bool seasonLimited = true;
	uint64_t claimDedupKey = dedupKey;
	if (seasonLimited) {
		claimDedupKey = HashGuildRewardKey(
		    ProgressionState.guildId.value,
		    ProgressionState.seasonId,
		    definition.activity,
		    definition.milestone,
		    definition.rewardId);
	}
	if (HasClaimDedupKey(claimDedupKey))
		return;

	AddDedupKey(dedupKey);
	AddClaimDedupKey(claimDedupKey);
	GrantedRewards.push_back(GrantedGuildReward {
	    dedupKey,
	    claimDedupKey,
	    definition.activity,
	    definition.milestone,
	    definition.rewardQuantity,
	    GuildRewardKind::TokenCurrency,
	    definition.rewardQuantity,
	    CalculateGuildSeasonTier(),
	    ProgressionState.seasonId,
	    seasonLimited,
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
	ProgressionState.seasonId = std::max(ProgressionState.seasonId, 1u);
	ProgressionState.seasonResetIntervalWeeks = std::max(ProgressionState.seasonResetIntervalWeeks, 1u);
	ProgressionState.nextSeasonResetWeek = std::max(ProgressionState.nextSeasonResetWeek, ProgressionState.seasonResetIntervalWeeks);
	ProgressionState.usedDedupKeys = std::min<uint16_t>(ProgressionState.usedDedupKeys, ProgressionState.dedupKeys.size());
	ProgressionState.usedClaimDedupKeys = std::min<uint16_t>(ProgressionState.usedClaimDedupKeys, ProgressionState.claimDedupKeys.size());
	ProgressionState.usedRankingSnapshots = std::min<uint8_t>(ProgressionState.usedRankingSnapshots, ProgressionState.rankingSnapshots.size());
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

uint16_t CalculateGuildSeasonTier()
{
	const uint32_t score = CalculateAccumulatedSeasonActivity();
	if (score >= 600)
		return 5;
	if (score >= 300)
		return 4;
	if (score >= 150)
		return 3;
	if (score >= 50)
		return 2;
	return score > 0 ? 1 : 0;
}

void CaptureGuildRankingSnapshot(uint32_t snapshotId)
{
	GuildRankingSnapshot snapshot {};
	snapshot.seasonId = ProgressionState.seasonId;
	snapshot.snapshotId = snapshotId;
	snapshot.accumulatedActivity = CalculateAccumulatedSeasonActivity();
	snapshot.tier = CalculateGuildSeasonTier();
	snapshot.prestigePoints = ProgressionState.prestigePoints;

	if (ProgressionState.usedRankingSnapshots < ProgressionState.rankingSnapshots.size()) {
		ProgressionState.rankingSnapshots[ProgressionState.usedRankingSnapshots++] = snapshot;
		return;
	}

	std::rotate(ProgressionState.rankingSnapshots.begin(), ProgressionState.rankingSnapshots.begin() + 1, ProgressionState.rankingSnapshots.end());
	ProgressionState.rankingSnapshots.back() = snapshot;
}

void AdvanceGuildSeasonIfNeeded(uint32_t currentWeek)
{
	if (ProgressionState.seasonResetIntervalWeeks == 0)
		return;
	if (ProgressionState.nextSeasonResetWeek == 0)
		ProgressionState.nextSeasonResetWeek = ProgressionState.seasonResetIntervalWeeks;

	while (currentWeek >= ProgressionState.nextSeasonResetWeek) {
		const uint32_t seasonActivity = CalculateAccumulatedSeasonActivity();
		ProgressionState.prestigePoints = std::min<uint32_t>(UINT32_MAX, ProgressionState.prestigePoints + seasonActivity);
		const uint16_t prestigeFromPoints = static_cast<uint16_t>(std::min<uint32_t>(UINT16_MAX, ProgressionState.prestigePoints / 500));
		ProgressionState.prestigeLevel = std::max(ProgressionState.prestigeLevel, prestigeFromPoints);

		ProgressionState.seasonTransitions = std::min<uint32_t>(UINT32_MAX, ProgressionState.seasonTransitions + 1);
		ProgressionState.seasonId = std::min<uint32_t>(UINT32_MAX, ProgressionState.seasonId + 1);
		ProgressionState.seasonCounterBaseline = ProgressionState.counters;
		ProgressionState.completedMilestones.fill(0);
		ProgressionState.usedClaimDedupKeys = 0;
		ProgressionState.nextSeasonResetWeek = std::min<uint32_t>(UINT32_MAX, ProgressionState.nextSeasonResetWeek + ProgressionState.seasonResetIntervalWeeks);
	}
}

} // namespace devilution
