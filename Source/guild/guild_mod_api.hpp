#pragma once

#include <algorithm>
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

namespace guild_mod_api_detail {

inline GuildModHooksV1 ActiveHooks {};
inline GuildModCompatibilityStatus Compatibility {};

inline void WriteModuleName(std::string_view moduleName)
{
	Compatibility.moduleName.fill('\0');
	const size_t count = std::min(moduleName.size(), Compatibility.moduleName.size() - 1);
	std::copy_n(moduleName.begin(), count, Compatibility.moduleName.begin());
}

} // namespace guild_mod_api_detail

inline void ResetGuildModHooks()
{
	guild_mod_api_detail::ActiveHooks = {};
	guild_mod_api_detail::Compatibility = {};
}

[[nodiscard]] inline bool RegisterGuildModHooks(std::string_view moduleName, uint32_t apiVersion, const GuildModHooksV1 &hooks)
{
	guild_mod_api_detail::Compatibility.requestedVersion = apiVersion;
	guild_mod_api_detail::Compatibility.activeVersion = CurrentGuildModApiVersion;
	guild_mod_api_detail::WriteModuleName(moduleName);
	if (apiVersion != CurrentGuildModApiVersion) {
		guild_mod_api_detail::Compatibility.compatible = false;
		return false;
	}
	guild_mod_api_detail::Compatibility.compatible = true;
	guild_mod_api_detail::ActiveHooks = hooks;
	return true;
}

[[nodiscard]] inline GuildModCompatibilityStatus GetGuildModCompatibilityStatus()
{
	return guild_mod_api_detail::Compatibility;
}

inline void NotifyGuildHallChanged(const GuildHallState &state)
{
	if (guild_mod_api_detail::ActiveHooks.onGuildHallChanged != nullptr)
		guild_mod_api_detail::ActiveHooks.onGuildHallChanged(state);
}

inline void NotifyGuildMemberChanged(uint8_t playerId, const GuildMemberState &state)
{
	if (guild_mod_api_detail::ActiveHooks.onGuildMemberChanged != nullptr)
		guild_mod_api_detail::ActiveHooks.onGuildMemberChanged(playerId, state);
}

} // namespace devilution
