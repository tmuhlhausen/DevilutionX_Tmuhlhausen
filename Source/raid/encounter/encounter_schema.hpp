#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "raid/encounter/encounter_engine.hpp"

namespace devilution {

enum class EncounterSchemaFormat : uint8_t {
	Toml,
	Json,
};

struct EncounterRewardDef {
	std::string rewardId;
	uint32_t quantity = 0;
};

struct EncounterSchemaDefinition {
	EncounterDefinition encounter;
	std::vector<EncounterRewardDef> rewards;
};

struct EncounterSchemaError {
	size_t line = 0;
	std::string message;
};

struct EncounterSchemaLoadResult {
	std::optional<EncounterSchemaDefinition> definition;
	std::vector<EncounterSchemaError> errors;
};

EncounterSchemaLoadResult LoadEncounterSchema(std::string_view source, EncounterSchemaFormat format);
std::vector<EncounterSchemaError> ValidateEncounterSchema(const EncounterSchemaDefinition &definition);

} // namespace devilution
