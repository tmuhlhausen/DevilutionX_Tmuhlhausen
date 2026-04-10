#include "raid/raid.hpp"

#include <algorithm>

#include "guild/guild_progression.hpp"
#include "levels/gendung.h"
#include "player.h"

namespace devilution {
namespace {

RaidInstanceState ActiveRaid {};
RaidLobbyUiState ActiveRaidLobbyUi {};

bool IsJoinablePhase(RaidPhase phase)
{
	return phase == RaidPhase::Inactive || phase == RaidPhase::Forming;
}

bool IsTerminalPhase(RaidPhase phase)
{
	return phase == RaidPhase::Completed || phase == RaidPhase::Failed || phase == RaidPhase::LockedOut;
}

void BumpRevision(RaidInstanceState &state)
{
	state.snapshotRevision++;
}

uint8_t ResolveGuildLevel(GuildId guildId)
{
	uint8_t guildLevel = 0;
	for (const Player &player : Players) {
		if (!player.plractive)
			continue;
		if (player.guildMemberState.guildId != guildId)
			continue;
		guildLevel = std::max(guildLevel, player.pNephilimLevel);
	}
	return guildLevel;
}

} // namespace

bool CanJoinRaid(const RaidInstanceState &state, const RaidMemberSnapshot &member, uint8_t activeMemberCount, uint8_t maxMembers)
{
	if (!member.isAlive)
		return false;
	if ((member.roleFlags & (RaidRoleTank | RaidRoleHealer | RaidRoleDamage | RaidRoleSupport)) == 0)
		return false;
	if (activeMemberCount >= maxMembers)
		return false;
	if (state.lockoutState == RaidLockoutState::Active)
		return false;
	if (!IsJoinablePhase(state.phase))
		return false;
	return !IsTerminalPhase(state.phase);
}

bool CanStartRaid(const RaidInstanceState &state, uint8_t readyMemberCount, uint8_t minimumMemberCount)
{
	if (state.lockoutState == RaidLockoutState::Active)
		return false;
	if (state.phase != RaidPhase::Forming && state.phase != RaidPhase::Inactive)
		return false;
	if (readyMemberCount < minimumMemberCount)
		return false;
	return state.raidId.IsValid() && state.difficulty != RaidDifficulty::None;
}

bool ApplyEncounterEvent(RaidInstanceState &state, const RaidEncounterEvent &event)
{
	if (event.bossIndex >= state.bossStates.size())
		return false;
	if (state.phase != RaidPhase::InProgress)
		return false;
	if (state.lockoutState == RaidLockoutState::Active)
		return false;

	state.bossStates[event.bossIndex] = event.state;
	state.objectiveBits |= event.objectiveBitsToSet;
	if (event.updateTimers)
		state.timersMs = event.timersMs;
	BumpRevision(state);
	return true;
}

bool CompleteRaid(RaidInstanceState &state, uint32_t lockoutExpirationTick)
{
	if (state.phase != RaidPhase::InProgress)
		return false;

	state.phase = RaidPhase::Completed;
	state.lockoutState = RaidLockoutState::Active;
	state.lockoutExpirationTick = lockoutExpirationTick;
	std::fill(state.bossStates.begin(), state.bossStates.end(), RaidEncounterState::Defeated);
	const GuildId guildId { ActiveGuildId };
	const uint8_t guildLevel = ResolveGuildLevel(guildId);
	HandleRaidCompletionForGuild(state.raidId.value, guildId, guildLevel, static_cast<uint8_t>(state.bossStates.size()));
	BumpRevision(state);
	return true;
}

bool FailRaid(RaidInstanceState &state, uint32_t lockoutExpirationTick)
{
	if (state.phase != RaidPhase::InProgress)
		return false;

	state.phase = RaidPhase::Failed;
	state.lockoutState = RaidLockoutState::Active;
	state.lockoutExpirationTick = lockoutExpirationTick;
	BumpRevision(state);
	return true;
}

void ResetRaid(RaidInstanceState &state, uint32_t newInstanceSeed)
{
	state.phase = RaidPhase::Inactive;
	state.lockoutState = RaidLockoutState::None;
	state.instanceSeed = newInstanceSeed;
	state.objectiveBits = 0;
	state.lockoutExpirationTick = 0;
	state.bossStates.fill(RaidEncounterState::NotStarted);
	state.timersMs.fill(0);
	BumpRevision(state);
}

void ResetActiveRaidState()
{
	ActiveRaid = {};
	ActiveRaidLobbyUi = {};
}

RaidInstanceState GetActiveRaidState()
{
	return ActiveRaid;
}

void ApplyActiveRaidStateSnapshot(const RaidInstanceState &state)
{
	ActiveRaid = state;
}

RaidLobbyUiState GetActiveRaidLobbyUiState()
{
	return ActiveRaidLobbyUi;
}

void ApplyActiveRaidLobbyUiState(const RaidLobbyUiState &state)
{
	ActiveRaidLobbyUi = state;
}

} // namespace devilution
