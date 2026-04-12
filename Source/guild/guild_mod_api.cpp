#include "guild/guild_mod_api.hpp"

#include <algorithm>

namespace devilution {
namespace {

GuildModHooksV1 ActiveHooks {};
GuildModCompatibilityStatus Compatibility {};

void WriteModuleName(std::string_view moduleName)
{
	Compatibility.moduleName.fill('\0');
	const size_t count = std::min(moduleName.size(), Compatibility.moduleName.size() - 1);
	std::copy_n(moduleName.begin(), count, Compatibility.moduleName.begin());
}

} // namespace

void ResetGuildModHooks()
{
	ActiveHooks = {};
	Compatibility = {};
}

bool RegisterGuildModHooks(std::string_view moduleName, uint32_t apiVersion, const GuildModHooksV1 &hooks)
{
	Compatibility.requestedVersion = apiVersion;
	Compatibility.activeVersion = CurrentGuildModApiVersion;
	WriteModuleName(moduleName);
	if (apiVersion != CurrentGuildModApiVersion) {
		Compatibility.compatible = false;
		return false;
	}
	Compatibility.compatible = true;
	ActiveHooks = hooks;
	return true;
}

GuildModCompatibilityStatus GetGuildModCompatibilityStatus()
{
	return Compatibility;
}

void NotifyGuildHallChanged(const GuildHallState &state)
{
	if (ActiveHooks.onGuildHallChanged != nullptr)
		ActiveHooks.onGuildHallChanged(state);
}

void NotifyGuildMemberChanged(uint8_t playerId, const GuildMemberState &state)
{
	if (ActiveHooks.onGuildMemberChanged != nullptr)
		ActiveHooks.onGuildMemberChanged(playerId, state);
}

} // namespace devilution
