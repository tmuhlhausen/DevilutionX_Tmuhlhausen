#include "raid/raid_state.hpp"

#include <algorithm>
#include <bit>

#include "guild/guild_progression.hpp"
#include "player.h"
#include "raid/raid_mod_api.hpp"
#include "raid/raid_rules.hpp"

namespace devilution {
namespace {

RaidInstanceState ActiveRaid {};

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

[[nodiscard]] bool IsValidRaidPlayerId(uint8_t playerId)
{
	return playerId < Players.size();
}

uint32_t PlayerBit(uint8_t playerId)
{
	return 1U << playerId;
}

} // namespace

void InitializeRaidSubsystem()
{
	ResetActiveRaidState();
	ResetRaidModHooks();
}

void ResetRaidSubsystem()
{
	ResetActiveRaidState();
	ResetRaidModHooks();
}

bool AdvanceRaidStateSequence(RaidInstanceState &state)
{
	state.snapshotRevision++;
	state.sequence++;
	return true;
}

uint8_t GetRaidMemberCount(const RaidInstanceState &state)
{
	return static_cast<uint8_t>(std::popcount(state.joinedMemberBits));
}

uint8_t GetRaidReadyCount(const RaidInstanceState &state)
{
	return static_cast<uint8_t>(std::popcount(state.readyMemberBits));
}

bool IsRaidMemberJoined(const RaidInstanceState &state, uint8_t playerId)
{
	if (!IsValidRaidPlayerId(playerId))
		return false;
	return (state.joinedMemberBits & PlayerBit(playerId)) != 0;
}

bool SetRaidMemberJoined(RaidInstanceState &state, uint8_t playerId, bool joined)
{
	if (!IsValidRaidPlayerId(playerId))
		return false;

	const uint32_t bit = PlayerBit(playerId);
	if (joined)
		state.joinedMemberBits |= bit;
	else
		state.joinedMemberBits &= ~bit;

	if (!joined)
		state.readyMemberBits &= ~bit;
	return true;
}

bool SetRaidMemberReady(RaidInstanceState &state, uint8_t playerId, bool ready)
{
	if (!IsValidRaidPlayerId(playerId) || !IsRaidMemberJoined(state, playerId))
		return false;

	const uint32_t bit = PlayerBit(playerId);
	if (ready)
		state.readyMemberBits |= bit;
	else
		state.readyMemberBits &= ~bit;
	return true;
}

bool ToggleRaidMemberReady(RaidInstanceState &state, uint8_t playerId)
{
	if (!IsValidRaidPlayerId(playerId) || !IsRaidMemberJoined(state, playerId))
		return false;

	state.readyMemberBits ^= PlayerBit(playerId);
	return true;
}

void ResetRaid(RaidInstanceState &state, uint32_t newInstanceSeed)
{
	if (!CanReset(state))
		return;
	state.phase = RaidPhase::Inactive;
	state.result = RaidResult::None;
	state.lockout = RaidLockout::None;
	state.instanceSeed = newInstanceSeed;
	state.objectiveBits = 0;
	state.checkpointBits = 0;
	state.lockoutExpirationTick = 0;
	state.joinedMemberBits = 0;
	state.readyMemberBits = 0;
	state.bossStates.fill(RaidEncounterState::NotStarted);
	state.timersMs.fill(0);
	AdvanceRaidStateSequence(state);
	NotifyRaidReset(state);
	NotifyRaidStateChanged(state);
}

void ResetActiveRaidState()
{
	ActiveRaid = {};
}

RaidInstanceState GetActiveRaidState()
{
	return ActiveRaid;
}

bool ApplyActiveRaidStateSnapshot(const RaidInstanceState &state)
{
	if (state.sequence < ActiveRaid.sequence)
		return false;
	if (state.snapshotRevision < ActiveRaid.snapshotRevision)
		return false;
	ActiveRaid = state;
	NotifyRaidStateChanged(ActiveRaid);
	return true;
}

bool ApplyEncounterEvent(RaidInstanceState &state, const RaidEncounterEvent &event)
{
	if (event.bossIndex >= state.bossStates.size())
		return false;
	if (state.phase != RaidPhase::InProgress)
		return false;
	if (state.lockout == RaidLockout::Active)
		return false;

	state.bossStates[event.bossIndex] = event.state;
	state.objectiveBits |= event.objectiveBitsToSet;
	state.checkpointBits |= event.checkpointBitsToSet;
	if (event.updateTimers)
		state.timersMs = event.timersMs;
	const bool advanced = AdvanceRaidStateSequence(state);
	if (advanced)
		NotifyRaidStateChanged(state);
	return advanced;
}

bool CompleteRaid(RaidInstanceState &state, uint32_t lockoutExpirationTick)
{
	if (state.phase != RaidPhase::InProgress || !CanProgressPhase(state.phase, RaidPhase::Completed))
		return false;

	state.phase = RaidPhase::Completed;
	state.result = RaidResult::Success;
	state.lockout = RaidLockout::Active;
	state.lockoutExpirationTick = lockoutExpirationTick;
	std::fill(state.bossStates.begin(), state.bossStates.end(), RaidEncounterState::Defeated);
	const GuildId guildId { ActiveGuildId };
	const uint8_t guildLevel = ResolveGuildLevel(guildId);
	HandleRaidCompletionForGuild(state.raidId.value, guildId, guildLevel, static_cast<uint8_t>(state.bossStates.size()));
	const bool advanced = AdvanceRaidStateSequence(state);
	if (advanced) {
		NotifyRaidCompleted(state);
		NotifyRaidStateChanged(state);
	}
	return advanced;
}

bool FailRaid(RaidInstanceState &state, uint32_t lockoutExpirationTick)
{
	if (state.phase != RaidPhase::InProgress || !CanProgressPhase(state.phase, RaidPhase::Failed))
		return false;

	state.phase = RaidPhase::Failed;
	state.result = RaidResult::Failure;
	state.lockout = RaidLockout::Active;
	state.lockoutExpirationTick = lockoutExpirationTick;
	const bool advanced = AdvanceRaidStateSequence(state);
	if (advanced) {
		NotifyRaidFailed(state);
		NotifyRaidStateChanged(state);
	}
	return advanced;
}

} // namespace devilution
