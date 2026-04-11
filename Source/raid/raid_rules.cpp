#include "raid/raid_rules.hpp"

namespace devilution {

namespace {

bool IsJoinablePhase(RaidPhase phase)
{
	return phase == RaidPhase::Inactive || phase == RaidPhase::Forming;
}

bool IsTerminalPhase(RaidPhase phase)
{
	return phase == RaidPhase::Completed || phase == RaidPhase::Failed || phase == RaidPhase::LockedOut;
}

} // namespace

bool CanJoin(const RaidInstanceState &state, const RaidMemberSnapshot &member, uint8_t activeMemberCount, uint8_t maxMembers)
{
	if (!member.isAlive)
		return false;
	if ((member.roleFlags & (RaidRoleTank | RaidRoleHealer | RaidRoleDamage | RaidRoleSupport)) == 0)
		return false;
	if (activeMemberCount >= maxMembers)
		return false;
	if (state.lockout == RaidLockout::Active)
		return false;
	if (!IsJoinablePhase(state.phase))
		return false;
	return !IsTerminalPhase(state.phase);
}

bool CanStart(const RaidInstanceState &state, uint8_t readyMemberCount, uint8_t minimumMemberCount)
{
	if (state.lockout == RaidLockout::Active)
		return false;
	if (state.phase != RaidPhase::Forming && state.phase != RaidPhase::Inactive)
		return false;
	if (readyMemberCount < minimumMemberCount)
		return false;
	return state.raidId.IsValid() && state.difficulty != RaidDifficulty::None;
}

bool CanProgressPhase(RaidPhase current, RaidPhase next)
{
	switch (current) {
	case RaidPhase::Inactive:
		return next == RaidPhase::Forming;
	case RaidPhase::Forming:
		return next == RaidPhase::InProgress || next == RaidPhase::Inactive;
	case RaidPhase::InProgress:
		return next == RaidPhase::Completed || next == RaidPhase::Failed;
	case RaidPhase::Completed:
	case RaidPhase::Failed:
	case RaidPhase::LockedOut:
		return next == RaidPhase::Inactive;
	}
	return false;
}

bool CanReset(const RaidInstanceState &state)
{
	return state.phase != RaidPhase::InProgress;
}

bool CanJoinRaid(const RaidInstanceState &state, const RaidMemberSnapshot &member, uint8_t activeMemberCount, uint8_t maxMembers)
{
	return CanJoin(state, member, activeMemberCount, maxMembers);
}

bool CanStartRaid(const RaidInstanceState &state, uint8_t readyMemberCount, uint8_t minimumMemberCount)
{
	return CanStart(state, readyMemberCount, minimumMemberCount);
}

} // namespace devilution
