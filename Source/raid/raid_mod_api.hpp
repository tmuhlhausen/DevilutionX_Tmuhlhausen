#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "raid/raid_state.hpp"

namespace devilution {

constexpr uint32_t RaidModApiVersion1 = 1;
constexpr uint32_t CurrentRaidModApiVersion = RaidModApiVersion1;

struct RaidModCompatibilityStatus {
	bool compatible = true;
	uint32_t requestedVersion = CurrentRaidModApiVersion;
	uint32_t activeVersion = CurrentRaidModApiVersion;
	std::array<char, 32> moduleName {};
};

struct RaidModHooksV1 {
	void (*onRaidStateChanged)(const RaidInstanceState &state) = nullptr;
	void (*onRaidReset)(const RaidInstanceState &state) = nullptr;
	void (*onRaidCompleted)(const RaidInstanceState &state) = nullptr;
	void (*onRaidFailed)(const RaidInstanceState &state) = nullptr;
};

namespace raid_mod_api_detail {

inline RaidModHooksV1 ActiveHooks {};
inline RaidModCompatibilityStatus Compatibility {};

inline void WriteModuleName(std::string_view moduleName)
{
	Compatibility.moduleName.fill('\0');
	const size_t count = std::min(moduleName.size(), Compatibility.moduleName.size() - 1);
	std::copy_n(moduleName.begin(), count, Compatibility.moduleName.begin());
}

inline void Invoke(void (*hook)(const RaidInstanceState &), const RaidInstanceState &state)
{
	if (hook != nullptr)
		hook(state);
}

} // namespace raid_mod_api_detail

inline void ResetRaidModHooks()
{
	raid_mod_api_detail::ActiveHooks = {};
	raid_mod_api_detail::Compatibility = {};
}

[[nodiscard]] inline bool RegisterRaidModHooks(std::string_view moduleName, uint32_t apiVersion, const RaidModHooksV1 &hooks)
{
	raid_mod_api_detail::Compatibility.requestedVersion = apiVersion;
	raid_mod_api_detail::Compatibility.activeVersion = CurrentRaidModApiVersion;
	raid_mod_api_detail::WriteModuleName(moduleName);
	if (apiVersion != CurrentRaidModApiVersion) {
		raid_mod_api_detail::Compatibility.compatible = false;
		return false;
	}
	raid_mod_api_detail::Compatibility.compatible = true;
	raid_mod_api_detail::ActiveHooks = hooks;
	return true;
}

[[nodiscard]] inline RaidModCompatibilityStatus GetRaidModCompatibilityStatus()
{
	return raid_mod_api_detail::Compatibility;
}

inline void NotifyRaidStateChanged(const RaidInstanceState &state)
{
	raid_mod_api_detail::Invoke(raid_mod_api_detail::ActiveHooks.onRaidStateChanged, state);
}

inline void NotifyRaidReset(const RaidInstanceState &state)
{
	raid_mod_api_detail::Invoke(raid_mod_api_detail::ActiveHooks.onRaidReset, state);
}

inline void NotifyRaidCompleted(const RaidInstanceState &state)
{
	raid_mod_api_detail::Invoke(raid_mod_api_detail::ActiveHooks.onRaidCompleted, state);
}

inline void NotifyRaidFailed(const RaidInstanceState &state)
{
	raid_mod_api_detail::Invoke(raid_mod_api_detail::ActiveHooks.onRaidFailed, state);
}

} // namespace devilution
