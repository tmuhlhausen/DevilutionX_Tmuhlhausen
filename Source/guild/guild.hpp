#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace devilution {

constexpr size_t MaxGuildNameLength = 24;

enum class MemberRole : uint8_t {
	None,
	Member,
	Officer,
	Leader,
};

enum GuildPermission : uint32_t {
	GuildPermissionNone = 0,
	GuildPermissionInvite = 1 << 0,
	GuildPermissionPromote = 1 << 1,
	GuildPermissionKick = 1 << 2,
	GuildPermissionManageHall = 1 << 3,
};

struct GuildId {
	uint32_t value = 0;

	[[nodiscard]] bool IsValid() const
	{
		return value != 0;
	}

	[[nodiscard]] bool operator==(const GuildId &other) const = default;
};

struct GuildName {
	std::array<char, MaxGuildNameLength> value {};
};

struct GuildMemberState {
	GuildId guildId {};
	MemberRole role = MemberRole::None;
	uint32_t permissions = GuildPermissionNone;
	bool invited = false;
};

struct GuildHallState {
	GuildId guildId {};
	GuildName name {};
	uint8_t memberCount = 0;
	uint8_t onlineCount = 0;
	bool isActive = false;
};

bool ValidateGuildName(std::string_view guildName);
uint32_t PermissionsForRole(MemberRole role);
bool IsRoleAtLeast(MemberRole role, MemberRole minimumRole);

void ResetGuildState();
GuildHallState GetGuildHallState();
GuildMemberState GetGuildMemberState(uint8_t playerId);
bool IsGuildRateLimited(uint8_t playerId, uint8_t actionKey, uint32_t minIntervalMs);

bool CreateGuild(uint8_t creatorPlayerId, std::string_view guildName);
bool InviteToGuild(uint8_t inviterPlayerId, uint8_t targetPlayerId);
bool JoinGuild(uint8_t playerId);
bool LeaveGuild(uint8_t playerId);
bool PromoteGuildMember(uint8_t promoterPlayerId, uint8_t targetPlayerId);
bool KickGuildMember(uint8_t kickerPlayerId, uint8_t targetPlayerId);

void SaveGuildMemberState(uint8_t playerId, GuildId guildId, MemberRole role, uint32_t permissions, bool invited);
void ApplyGuildHallSnapshot(const GuildHallState &state);

} // namespace devilution
