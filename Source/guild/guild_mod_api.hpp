#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "guild/guild.hpp"

namespace devilution {

constexpr uint32_t GuildModApiVersion1 = 1;
constexpr uint32_t CurrentGuildModApiVersion = GuildModApiVersion1;

struct GuildModCompatibilityStatus {
	bool compatible = true;
	uint32_t requestedVersion = CurrentGuildModApiVersion;
	uint32_t activeVersion = CurrentGuildModApiVersion;
	std::array<char, 32> moduleName {};
};

struct GuildModHooksV1 {
	void (*onGuildHallChanged)(const GuildHallState &state) = nullptr;
	void (*onGuildMemberChanged)(uint8_t playerId, const GuildMemberState &state) = nullptr;
};

void ResetGuildModHooks();
[[nodiscard]] bool RegisterGuildModHooks(std::string_view moduleName, uint32_t apiVersion, const GuildModHooksV1 &hooks);
[[nodiscard]] GuildModCompatibilityStatus GetGuildModCompatibilityStatus();

void NotifyGuildHallChanged(const GuildHallState &state);
void NotifyGuildMemberChanged(uint8_t playerId, const GuildMemberState &state);

} // namespace devilution
