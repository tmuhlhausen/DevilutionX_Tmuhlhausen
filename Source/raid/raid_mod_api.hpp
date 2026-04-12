#pragma once

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

void ResetRaidModHooks();
[[nodiscard]] bool RegisterRaidModHooks(std::string_view moduleName, uint32_t apiVersion, const RaidModHooksV1 &hooks);
[[nodiscard]] RaidModCompatibilityStatus GetRaidModCompatibilityStatus();

void NotifyRaidStateChanged(const RaidInstanceState &state);
void NotifyRaidReset(const RaidInstanceState &state);
void NotifyRaidCompleted(const RaidInstanceState &state);
void NotifyRaidFailed(const RaidInstanceState &state);

} // namespace devilution
