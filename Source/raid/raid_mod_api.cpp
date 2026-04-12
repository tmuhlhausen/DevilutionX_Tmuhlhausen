#include "raid/raid_mod_api.hpp"

#include <algorithm>

namespace devilution {
namespace {

RaidModHooksV1 ActiveHooks {};
RaidModCompatibilityStatus Compatibility {};

void WriteModuleName(std::string_view moduleName)
{
	Compatibility.moduleName.fill('\0');
	const size_t count = std::min(moduleName.size(), Compatibility.moduleName.size() - 1);
	std::copy_n(moduleName.begin(), count, Compatibility.moduleName.begin());
}

void Invoke(void (*hook)(const RaidInstanceState &), const RaidInstanceState &state)
{
	if (hook != nullptr)
		hook(state);
}

} // namespace

void ResetRaidModHooks()
{
	ActiveHooks = {};
	Compatibility = {};
}

bool RegisterRaidModHooks(std::string_view moduleName, uint32_t apiVersion, const RaidModHooksV1 &hooks)
{
	Compatibility.requestedVersion = apiVersion;
	Compatibility.activeVersion = CurrentRaidModApiVersion;
	WriteModuleName(moduleName);
	if (apiVersion != CurrentRaidModApiVersion) {
		Compatibility.compatible = false;
		return false;
	}
	Compatibility.compatible = true;
	ActiveHooks = hooks;
	return true;
}

RaidModCompatibilityStatus GetRaidModCompatibilityStatus()
{
	return Compatibility;
}

void NotifyRaidStateChanged(const RaidInstanceState &state)
{
	Invoke(ActiveHooks.onRaidStateChanged, state);
}

void NotifyRaidReset(const RaidInstanceState &state)
{
	Invoke(ActiveHooks.onRaidReset, state);
}

void NotifyRaidCompleted(const RaidInstanceState &state)
{
	Invoke(ActiveHooks.onRaidCompleted, state);
}

void NotifyRaidFailed(const RaidInstanceState &state)
{
	Invoke(ActiveHooks.onRaidFailed, state);
}

} // namespace devilution
