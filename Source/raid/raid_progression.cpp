#include "raid/raid_progression.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace devilution {
namespace {

RaidProgressionPersistedState ProgressionState {};
std::vector<RaidRewardClaim> GrantedRaidRewards {};

constexpr size_t DifficultyIndex(RaidDifficulty difficulty)
{
	if (difficulty == RaidDifficulty::Normal)
		return 0;
	if (difficulty == RaidDifficulty::Nightmare)
		return 1;
	return 2;
}

uint64_t HashRewardToken(uint32_t guildId, uint8_t playerId, uint32_t raidId, RaidDifficulty difficulty, GuildActivityType activity, uint16_t milestone)
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
	hashByte(playerId);
	hashU32(raidId);
	hashByte(static_cast<uint8_t>(difficulty));
	hashByte(static_cast<uint8_t>(activity));
	hashByte(static_cast<uint8_t>(milestone & 0xFF));
	hashByte(static_cast<uint8_t>((milestone >> 8) & 0xFF));
	return hash;
}

bool HasClaimToken(const std::array<RaidRewardClaim, MaxRaidPlayerRewardClaims> &claims, uint16_t count, uint64_t token)
{
	const size_t checkedCount = std::min<size_t>(count, claims.size());
	for (size_t i = 0; i < checkedCount; i++) {
		if (claims[i].token == token)
			return true;
	}
	return false;
}

bool HasClaimToken(const std::array<RaidRewardClaim, MaxRaidGuildRewardClaims> &claims, uint16_t count, uint64_t token)
{
	const size_t checkedCount = std::min<size_t>(count, claims.size());
	for (size_t i = 0; i < checkedCount; i++) {
		if (claims[i].token == token)
			return true;
	}
	return false;
}

template <typename ClaimArray>
void PushClaim(ClaimArray &claims, uint16_t &usedClaims, const RaidRewardClaim &claim)
{
	if (usedClaims < claims.size()) {
		claims[usedClaims++] = claim;
		return;
	}

	std::rotate(claims.begin(), claims.begin() + 1, claims.end());
	claims.back() = claim;
}

void ResolveMilestoneRewardsForParticipants(uint32_t raidId, RaidDifficulty difficulty, GuildId guildId, const std::vector<uint8_t> &participantIds, size_t initialRewardCount)
{
	const std::vector<GrantedGuildReward> &grantedGuildRewards = GetGrantedGuildRewards();
	if (grantedGuildRewards.size() <= initialRewardCount)
		return;

	for (size_t rewardIndex = initialRewardCount; rewardIndex < grantedGuildRewards.size(); rewardIndex++) {
		const GrantedGuildReward &reward = grantedGuildRewards[rewardIndex];
		const uint64_t guildToken = HashRewardToken(guildId.value, UINT8_MAX, raidId, difficulty, reward.activity, reward.milestone);
		if (!HasClaimToken(ProgressionState.guildClaims, ProgressionState.usedGuildClaims, guildToken)) {
			RaidRewardClaim guildClaim {};
			guildClaim.token = guildToken;
			guildClaim.guildId = guildId.value;
			guildClaim.raidId = raidId;
			guildClaim.playerId = UINT8_MAX;
			guildClaim.activity = reward.activity;
			guildClaim.milestone = reward.milestone;
			guildClaim.quantity = reward.quantity;
			PushClaim(ProgressionState.guildClaims, ProgressionState.usedGuildClaims, guildClaim);
		}

		for (uint8_t playerId : participantIds) {
			const uint64_t playerToken = HashRewardToken(guildId.value, playerId, raidId, difficulty, reward.activity, reward.milestone);
			if (HasClaimToken(ProgressionState.playerClaims, ProgressionState.usedPlayerClaims, playerToken))
				continue;
			RaidRewardClaim claim {};
			claim.token = playerToken;
			claim.guildId = guildId.value;
			claim.raidId = raidId;
			claim.playerId = playerId;
			claim.activity = reward.activity;
			claim.milestone = reward.milestone;
			claim.quantity = reward.quantity;
			PushClaim(ProgressionState.playerClaims, ProgressionState.usedPlayerClaims, claim);
			GrantedRaidRewards.push_back(claim);
		}
	}
}

} // namespace

void ResetRaidProgression()
{
	ProgressionState = {};
	GrantedRaidRewards.clear();
}

RaidProgressionPersistedState GetRaidProgressionPersistedState()
{
	return ProgressionState;
}

void ApplyRaidProgressionPersistedState(const RaidProgressionPersistedState &state)
{
	ProgressionState = state;
	ProgressionState.usedPlayerClaims = std::min<uint16_t>(ProgressionState.usedPlayerClaims, ProgressionState.playerClaims.size());
	ProgressionState.usedGuildClaims = std::min<uint16_t>(ProgressionState.usedGuildClaims, ProgressionState.guildClaims.size());
}

bool BeginRaidAttempt(RaidDifficulty difficulty, uint32_t currentWeek)
{
	if (difficulty == RaidDifficulty::None)
		return false;

	RaidDifficultyProgressState &state = ProgressionState.difficulties[DifficultyIndex(difficulty)];
	if (state.lockoutWeek == currentWeek)
		return false;
	if (state.lockoutWeek != 0 && state.lockoutWeek != currentWeek)
		state.attemptsThisWeek = 0;
	state.lockoutWeek = currentWeek;
	state.attemptsThisWeek = std::min<uint16_t>(UINT16_MAX, state.attemptsThisWeek + 1);
	return true;
}

void SetRaidLockoutForWeek(RaidDifficulty difficulty, uint32_t currentWeek)
{
	if (difficulty == RaidDifficulty::None)
		return;
	ProgressionState.difficulties[DifficultyIndex(difficulty)].lockoutWeek = currentWeek;
}

bool IsRaidDifficultyLocked(RaidDifficulty difficulty, uint32_t currentWeek)
{
	if (difficulty == RaidDifficulty::None)
		return false;
	return ProgressionState.difficulties[DifficultyIndex(difficulty)].lockoutWeek == currentWeek;
}

bool SaveRaidCheckpoint(RaidDifficulty difficulty, uint8_t bossIndex, uint64_t objectiveBits, const std::array<uint32_t, MaxRaidTimers> &timersMs)
{
	if (difficulty == RaidDifficulty::None)
		return false;
	RaidDifficultyProgressState &state = ProgressionState.difficulties[DifficultyIndex(difficulty)];
	state.checkpointBossIndex = bossIndex;
	state.checkpointObjectiveBits = objectiveBits;
	state.checkpointTimersMs = timersMs;
	return true;
}

void ClearRaidCheckpoint(RaidDifficulty difficulty)
{
	if (difficulty == RaidDifficulty::None)
		return;
	RaidDifficultyProgressState &state = ProgressionState.difficulties[DifficultyIndex(difficulty)];
	state.checkpointBossIndex = NoRaidCheckpointBoss;
	state.checkpointObjectiveBits = 0;
	state.checkpointTimersMs.fill(0);
}

void RecordRaidDungeonClear(uint32_t raidId, RaidDifficulty difficulty, GuildId guildId, uint8_t guildLevel, uint8_t bossesDefeated, uint16_t clearDurationSeconds, const std::vector<uint8_t> &participantIds, uint32_t currentWeek)
{
	if (!guildId.IsValid() || difficulty == RaidDifficulty::None)
		return;

	RaidDifficultyProgressState &state = ProgressionState.difficulties[DifficultyIndex(difficulty)];
	if (state.lockoutWeek != currentWeek)
		state.attemptsThisWeek = 0;
	state.lockoutWeek = currentWeek;
	if (state.bestClearDurationSeconds == 0 || clearDurationSeconds < state.bestClearDurationSeconds) {
		state.bestClearDurationSeconds = clearDurationSeconds;
		state.bestBossesDefeated = bossesDefeated;
		state.bestClearRaidId = raidId;
	}
	ClearRaidCheckpoint(difficulty);

	const size_t baseRewardCount = GetGrantedGuildRewards().size();
	EmitGuildActivityEvent(guildId, GuildActivityType::DungeonClear, 1, guildLevel, raidId);
	if (bossesDefeated != 0)
		EmitGuildActivityEvent(guildId, GuildActivityType::BossKill, bossesDefeated, guildLevel, raidId);
	if (GetGrantedGuildRewards().size() > baseRewardCount)
		ResolveMilestoneRewardsForParticipants(raidId, difficulty, guildId, participantIds, baseRewardCount);
}

const std::vector<RaidRewardClaim> &GetRaidRewardClaims()
{
	return GrantedRaidRewards;
}

} // namespace devilution
