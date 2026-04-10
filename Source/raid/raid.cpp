#include "raid/raid.hpp"

#include <algorithm>

namespace devilution {
namespace {

RaidInstanceState ActiveRaid {};

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
}

RaidInstanceState GetActiveRaidState()
{
	return ActiveRaid;
}

void ApplyActiveRaidStateSnapshot(const RaidInstanceState &state)
{
	ActiveRaid = state;
}

} // namespace devilution
