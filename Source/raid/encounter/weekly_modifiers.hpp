#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace devilution {

struct WeeklyModifierDef {
	uint16_t id = 0;
	std::string name;
	std::string description;
};

class WeeklyModifierRegistry {
public:
	void Register(const WeeklyModifierDef &modifier);
	void RegisterDefaults();
	[[nodiscard]] std::optional<WeeklyModifierDef> FindById(uint16_t id) const;
	[[nodiscard]] std::vector<uint16_t> GetWeeklyRotation(uint16_t season, uint16_t week, size_t count) const;

private:
	[[nodiscard]] uint32_t SeedForWeek(uint16_t season, uint16_t week) const;
	std::vector<WeeklyModifierDef> modifiers_;
};

} // namespace devilution
