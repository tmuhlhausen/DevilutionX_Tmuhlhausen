#include "guild/guild.hpp"

#include <algorithm>
#include <array>
#include <cctype>

#ifdef USE_SDL3
#include <SDL3/SDL_timer.h>
#else
#include <SDL_timer.h>
#endif

#include "guild/guild_mod_api.hpp"
#include "multi.h"
#include "player.h"

// TODO: Move this implementation into Source/CMakeLists.txt once the source list is refactored
// enough to make small, low-risk CMake changes. Keeping the implementation in this translation
// unit prevents unresolved guild mod hook symbols while preserving the existing build layout.
#include "guild/guild_mod_api.cpp"

namespace devilution {
namespace {

GuildHallState ActiveGuild {};
std::array<GuildMemberState, MAX_PLRS> MemberStates {};
std::array<std::array<uint32_t, 8>, MAX_PLRS> ActionTimestamps {};
GuildId NextGuildId { 1 };

bool IsAcceptedGuildMember(const GuildMemberState &state)
{
	return state.guildId.IsValid() && !state.invited && state.role != MemberRole::None;
}

void ClearGuild()
{
	ActiveGuild = {};
	for (GuildMemberState &state : MemberStates)
		state = {};
	NotifyGuildHallChanged(ActiveGuild);
}

void RecountGuildState()
{
	if (!ActiveGuild.guildId.IsValid()) {
		ActiveGuild = {};
		return;
	}

	uint8_t members = 0;
	uint8_t online = 0;
	for (size_t i = 0; i < MemberStates.size(); i++) {
		if (MemberStates[i].guildId != ActiveGuild.guildId || !IsAcceptedGuildMember(MemberStates[i]))
			continue;
		members++;
		if (Players[i].plractive)
			online++;
	}
	ActiveGuild.memberCount = members;
	ActiveGuild.onlineCount = online;
	ActiveGuild.isActive = members > 0;
	if (members == 0)
		ActiveGuild = {};
}

bool IsGuildMember(uint8_t playerId)
{
	return playerId < MAX_PLRS && IsAcceptedGuildMember(MemberStates[playerId]);
}

bool TryTransferOwnership(uint8_t previousLeader)
{
	if (previousLeader >= MAX_PLRS)
		return false;

	const GuildId guildId = MemberStates[previousLeader].guildId;
	if (!guildId.IsValid())
		return false;

	for (size_t i = 0; i < MemberStates.size(); i++) {
		if (i == previousLeader)
			continue;
		if (MemberStates[i].guildId == guildId && IsAcceptedGuildMember(MemberStates[i]) && MemberStates[i].role == MemberRole::Officer) {
			MemberStates[i].role = MemberRole::Leader;
			MemberStates[i].permissions = PermissionsForRole(MemberStates[i].role);
			return true;
		}
	}

	for (size_t i = 0; i < MemberStates.size(); i++) {
		if (i == previousLeader)
			continue;
		if (MemberStates[i].guildId == guildId && IsAcceptedGuildMember(MemberStates[i])) {
			MemberStates[i].role = MemberRole::Leader;
			MemberStates[i].permissions = PermissionsForRole(MemberStates[i].role);
			return true;
		}
	}

	return false;
}

void CopyGuildName(std::string_view name)
{
	std::fill(ActiveGuild.name.value.begin(), ActiveGuild.name.value.end(), '\0');
	const size_t copyLength = std::min(name.size(), ActiveGuild.name.value.size() - 1);
	std::copy(name.begin(), name.begin() + copyLength, ActiveGuild.name.value.begin());
}

} // namespace

bool ValidateGuildName(std::string_view guildName)
{
	if (guildName.size() < 3 || guildName.size() >= MaxGuildNameLength)
		return false;
	if (guildName.front() == ' ' || guildName.back() == ' ')
		return false;

	bool previousSpace = false;
	for (const char c : guildName) {
		if (c == ' ') {
			if (previousSpace)
				return false;
			previousSpace = true;
			continue;
		}
		previousSpace = false;
		if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
			return false;
	}
	return true;
}

uint32_t PermissionsForRole(MemberRole role)
{
	switch (role) {
	case MemberRole::Leader:
		return GuildPermissionInvite | GuildPermissionPromote | GuildPermissionKick | GuildPermissionManageHall;
	case MemberRole::Officer:
		return GuildPermissionInvite | GuildPermissionKick;
	case MemberRole::Member:
	case MemberRole::None:
		return GuildPermissionNone;
	}
	return GuildPermissionNone;
}

bool IsRoleAtLeast(MemberRole role, MemberRole minimumRole)
{
	return static_cast<uint8_t>(role) >= static_cast<uint8_t>(minimumRole);
}

void ResetGuildState()
{
	ClearGuild();
	ResetGuildModHooks();
	for (auto &timestamps : ActionTimestamps)
		timestamps.fill(0);
}

GuildHallState GetGuildHallState()
{
	RecountGuildState();
	return ActiveGuild;
}

GuildMemberState GetGuildMemberState(uint8_t playerId)
{
	if (playerId >= MAX_PLRS)
		return {};
	return MemberStates[playerId];
}

bool HasGuildInvite(uint8_t playerId)
{
	if (playerId >= MAX_PLRS)
		return false;
	const GuildMemberState &state = MemberStates[playerId];
	return state.guildId.IsValid() && state.invited && state.role == MemberRole::None;
}

bool IsGuildRateLimited(uint8_t playerId, uint8_t actionKey, uint32_t minIntervalMs)
{
	if (playerId >= MAX_PLRS || actionKey >= ActionTimestamps[playerId].size())
		return true;
	const uint32_t now = SDL_GetTicks();
	const uint32_t lastAction = ActionTimestamps[playerId][actionKey];
	if (lastAction != 0 && now - lastAction < minIntervalMs)
		return true;
	ActionTimestamps[playerId][actionKey] = now;
	return false;
}

bool HasGuildInvite(uint8_t playerId)
{
	if (playerId >= MAX_PLRS)
		return false;
	const GuildMemberState &state = MemberStates[playerId];
	return state.guildId.IsValid() && state.role == MemberRole::None && state.invited;
}

bool CreateGuild(uint8_t creatorPlayerId, std::string_view guildName)
{
	if (creatorPlayerId >= MAX_PLRS || IsGuildMember(creatorPlayerId) || HasGuildInvite(creatorPlayerId) || ActiveGuild.guildId.IsValid())
		return false;
	if (!ValidateGuildName(guildName))
		return false;

	ActiveGuild = {};
	ActiveGuild.guildId = NextGuildId;
	NextGuildId.value++;
	CopyGuildName(guildName);
	ActiveGuild.isActive = true;

	SaveGuildMemberState(creatorPlayerId, ActiveGuild.guildId, MemberRole::Leader, PermissionsForRole(MemberRole::Leader), false);
	RecountGuildState();
	NotifyGuildHallChanged(ActiveGuild);
	NotifyGuildMemberChanged(creatorPlayerId, MemberStates[creatorPlayerId]);
	return true;
}

bool InviteToGuild(uint8_t inviterPlayerId, uint8_t targetPlayerId)
{
	if (inviterPlayerId >= MAX_PLRS || targetPlayerId >= MAX_PLRS || inviterPlayerId == targetPlayerId)
		return false;
	if (!IsGuildMember(inviterPlayerId) || IsGuildMember(targetPlayerId) || HasGuildInvite(targetPlayerId))
		return false;

	const GuildMemberState inviter = MemberStates[inviterPlayerId];
	if ((inviter.permissions & GuildPermissionInvite) == 0)
		return false;
	SaveGuildMemberState(targetPlayerId, inviter.guildId, MemberRole::None, GuildPermissionNone, true);
	NotifyGuildMemberChanged(targetPlayerId, MemberStates[targetPlayerId]);
	return true;
}

bool JoinGuild(uint8_t playerId)
{
	if (playerId >= MAX_PLRS || IsGuildMember(playerId) || !HasGuildInvite(playerId))
		return false;
	SaveGuildMemberState(playerId, MemberStates[playerId].guildId, MemberRole::Member, PermissionsForRole(MemberRole::Member), false);
	RecountGuildState();
	NotifyGuildHallChanged(ActiveGuild);
	NotifyGuildMemberChanged(playerId, MemberStates[playerId]);
	return true;
}

bool LeaveGuild(uint8_t playerId)
{
	if (playerId >= MAX_PLRS || !IsGuildMember(playerId))
		return false;

	const MemberRole role = MemberStates[playerId].role;
	if (role == MemberRole::Leader)
		TryTransferOwnership(playerId);
	SaveGuildMemberState(playerId, {}, MemberRole::None, GuildPermissionNone, false);
	RecountGuildState();
	NotifyGuildHallChanged(ActiveGuild);
	NotifyGuildMemberChanged(playerId, MemberStates[playerId]);
	return true;
}

bool PromoteGuildMember(uint8_t promoterPlayerId, uint8_t targetPlayerId)
{
	if (promoterPlayerId >= MAX_PLRS || targetPlayerId >= MAX_PLRS || promoterPlayerId == targetPlayerId)
		return false;
	if (!IsGuildMember(promoterPlayerId) || !IsGuildMember(targetPlayerId))
		return false;

	const GuildMemberState &promoter = MemberStates[promoterPlayerId];
	GuildMemberState &target = MemberStates[targetPlayerId];
	if (promoter.guildId != target.guildId || (promoter.permissions & GuildPermissionPromote) == 0)
		return false;
	if (!IsRoleAtLeast(promoter.role, MemberRole::Leader) || target.role != MemberRole::Member)
		return false;

	target.role = MemberRole::Officer;
	target.permissions = PermissionsForRole(target.role);
	NotifyGuildMemberChanged(targetPlayerId, target);
	return true;
}

bool KickGuildMember(uint8_t kickerPlayerId, uint8_t targetPlayerId)
{
	if (kickerPlayerId >= MAX_PLRS || targetPlayerId >= MAX_PLRS || kickerPlayerId == targetPlayerId)
		return false;
	if (!IsGuildMember(kickerPlayerId) || !IsGuildMember(targetPlayerId))
		return false;

	const GuildMemberState kicker = MemberStates[kickerPlayerId];
	const GuildMemberState target = MemberStates[targetPlayerId];
	if (kicker.guildId != target.guildId || (kicker.permissions & GuildPermissionKick) == 0 || target.role == MemberRole::Leader)
		return false;

	SaveGuildMemberState(targetPlayerId, {}, MemberRole::None, GuildPermissionNone, false);
	RecountGuildState();
	NotifyGuildHallChanged(ActiveGuild);
	NotifyGuildMemberChanged(targetPlayerId, MemberStates[targetPlayerId]);
	return true;
}

void SaveGuildMemberState(uint8_t playerId, GuildId guildId, MemberRole role, uint32_t permissions, bool invited)
{
	if (playerId >= MAX_PLRS)
		return;
	MemberStates[playerId].guildId = guildId;
	MemberStates[playerId].role = role;
	MemberStates[playerId].permissions = permissions;
	MemberStates[playerId].invited = invited;

	Player &player = Players[playerId];
	player.guildMemberState.guildId = guildId;
	player.guildMemberState.role = role;
	player.guildMemberState.permissions = permissions;
	player.guildMemberState.invited = invited;
	NotifyGuildMemberChanged(playerId, MemberStates[playerId]);
}

void ApplyGuildHallSnapshot(const GuildHallState &state)
{
	ActiveGuild = state;
	NotifyGuildHallChanged(ActiveGuild);
	if (!ActiveGuild.guildId.IsValid())
		ClearGuild();
}

} // namespace devilution
