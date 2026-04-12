#include "raid/encounter/weekly_modifiers.hpp"

#include <algorithm>

namespace devilution {

void WeeklyModifierRegistry::Register(const WeeklyModifierDef &modifier)
{
	const auto it = std::find_if(modifiers_.begin(), modifiers_.end(), [&](const WeeklyModifierDef &entry) {
		return entry.id == modifier.id;
	});
	if (it == modifiers_.end())
		modifiers_.push_back(modifier);
}

void WeeklyModifierRegistry::RegisterDefaults()
{
	Register(WeeklyModifierDef { 1, "Volcanic", "Periodic magma eruptions spawn under players." });
	Register(WeeklyModifierDef { 2, "Overclocked", "Boss ability cooldowns are reduced." });
	Register(WeeklyModifierDef { 3, "Attrition", "Healing received is reduced while moving." });
	Register(WeeklyModifierDef { 4, "Arc Surge", "Arcane conduits rotate around arenas." });
	Register(WeeklyModifierDef { 5, "Nightfall", "Ambient visibility shrinks over time." });
}

std::optional<WeeklyModifierDef> WeeklyModifierRegistry::FindById(uint16_t id) const
{
	const auto it = std::find_if(modifiers_.begin(), modifiers_.end(), [&](const WeeklyModifierDef &entry) {
		return entry.id == id;
	});
	if (it == modifiers_.end())
		return std::nullopt;
	return *it;
}

uint32_t WeeklyModifierRegistry::SeedForWeek(uint16_t season, uint16_t week) const
{
	uint32_t state = (static_cast<uint32_t>(season) << 16) ^ static_cast<uint32_t>(week);
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return state;
}

std::vector<uint16_t> WeeklyModifierRegistry::GetWeeklyRotation(uint16_t season, uint16_t week, size_t count) const
{
	std::vector<uint16_t> ids;
	ids.reserve(modifiers_.size());
	for (const WeeklyModifierDef &mod : modifiers_)
		ids.push_back(mod.id);
	if (ids.empty() || count == 0)
		return {};

	uint32_t state = SeedForWeek(season, week);
	for (size_t i = ids.size(); i > 1; --i) {
		state = state * 1664525u + 1013904223u;
		const size_t swapIndex = state % i;
		std::swap(ids[i - 1], ids[swapIndex]);
	}
	if (count < ids.size())
		ids.resize(count);
	return ids;
}

} // namespace devilution
