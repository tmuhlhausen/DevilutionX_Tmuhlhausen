#include "raid/encounter/encounter_schema.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string>
#include <unordered_set>

namespace devilution {
namespace {

std::string Trim(std::string_view input)
{
	size_t start = 0;
	while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start])) != 0)
		++start;
	size_t end = input.size();
	while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0)
		--end;
	return std::string(input.substr(start, end - start));
}

bool ParseUInt(std::string_view text, uint32_t &value)
{
	auto trimmed = Trim(text);
	const char *begin = trimmed.data();
	const char *end = begin + trimmed.size();
	auto [ptr, ec] = std::from_chars(begin, end, value);
	return ec == std::errc() && ptr == end;
}

std::optional<EncounterConditionType> ParseConditionType(const std::string &value)
{
	if (value == "None")
		return EncounterConditionType::None;
	if (value == "TimeElapsedAtLeast")
		return EncounterConditionType::TimeElapsedAtLeast;
	if (value == "BossHealthAtMost")
		return EncounterConditionType::BossHealthAtMost;
	if (value == "AddsDefeatedAtLeast")
		return EncounterConditionType::AddsDefeatedAtLeast;
	if (value == "PlayerDeathsAtLeast")
		return EncounterConditionType::PlayerDeathsAtLeast;
	if (value == "AllPlayersDead")
		return EncounterConditionType::AllPlayersDead;
	return std::nullopt;
}

std::optional<EncounterFailurePolicy> ParseFailurePolicy(const std::string &value)
{
	if (value == "Wipe")
		return EncounterFailurePolicy::Wipe;
	if (value == "PartialFail")
		return EncounterFailurePolicy::PartialFail;
	if (value == "CheckpointRollback")
		return EncounterFailurePolicy::CheckpointRollback;
	return std::nullopt;
}

std::optional<EncounterMechanicType> ParseMechanicType(const std::string &value)
{
	if (value == "AoEPulse")
		return EncounterMechanicType::AoEPulse;
	if (value == "AddWave")
		return EncounterMechanicType::AddWave;
	if (value == "PositionalCheck")
		return EncounterMechanicType::PositionalCheck;
	if (value == "Enrage")
		return EncounterMechanicType::Enrage;
	return std::nullopt;
}

bool ParseConditionText(std::string_view text, EncounterCondition &condition)
{
	const auto colon = text.find(':');
	if (colon == std::string_view::npos)
		return false;
	const std::string conditionType = Trim(text.substr(0, colon));
	auto parsedType = ParseConditionType(conditionType);
	if (!parsedType)
		return false;
	uint32_t threshold = 0;
	if (!ParseUInt(text.substr(colon + 1), threshold))
		return false;
	condition.type = *parsedType;
	condition.threshold = threshold;
	return true;
}

std::string StripQuotes(std::string value)
{
	if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
		return value.substr(1, value.size() - 2);
	return value;
}

void AddError(std::vector<EncounterSchemaError> &errors, size_t line, const std::string &message)
{
	errors.push_back(EncounterSchemaError { line, message });
}

} // namespace

EncounterSchemaLoadResult LoadEncounterSchema(std::string_view source, EncounterSchemaFormat format)
{
	EncounterSchemaLoadResult result;
	EncounterSchemaDefinition def;
	enum class Section {
		Root,
		Phase,
		Mechanic,
		Reward,
	};
	Section section = Section::Root;

	std::istringstream stream(std::string(source));
	std::string line;
	size_t lineNo = 0;
	while (std::getline(stream, line)) {
		++lineNo;
		std::string trimmed = Trim(line);
		if (trimmed.empty() || trimmed[0] == '#')
			continue;
		if (format == EncounterSchemaFormat::Toml) {
			if (trimmed == "[[phases]]") {
				def.encounter.phases.emplace_back();
				section = Section::Phase;
				continue;
			}
			if (trimmed == "[[mechanics]]") {
				def.encounter.mechanics.emplace_back();
				section = Section::Mechanic;
				continue;
			}
			if (trimmed == "[[rewards]]") {
				def.rewards.emplace_back();
				section = Section::Reward;
				continue;
			}
		}
		if (format == EncounterSchemaFormat::Json) {
			if (trimmed.find("\"phases\"") != std::string::npos && trimmed.find('[') != std::string::npos) {
				section = Section::Phase;
				continue;
			}
			if (trimmed.find("\"mechanics\"") != std::string::npos && trimmed.find('[') != std::string::npos) {
				section = Section::Mechanic;
				continue;
			}
			if (trimmed.find("\"rewards\"") != std::string::npos && trimmed.find('[') != std::string::npos) {
				section = Section::Reward;
				continue;
			}
			if (trimmed == "{" && section == Section::Phase)
				def.encounter.phases.emplace_back();
			if (trimmed == "{" && section == Section::Mechanic)
				def.encounter.mechanics.emplace_back();
			if (trimmed == "{" && section == Section::Reward)
				def.rewards.emplace_back();
			if (trimmed == "]," || trimmed == "]")
				section = Section::Root;
		}

		auto eq = trimmed.find(format == EncounterSchemaFormat::Toml ? '=' : ':');
		if (eq == std::string::npos)
			continue;
		std::string key = Trim(trimmed.substr(0, eq));
		std::string value = Trim(trimmed.substr(eq + 1));
		if (!value.empty() && value.back() == ',')
			value.pop_back();
		key = StripQuotes(key);
		value = StripQuotes(value);

		auto parseUIntField = [&](uint32_t &target) {
			if (!ParseUInt(value, target)) {
				AddError(result.errors, lineNo, "Expected unsigned integer for " + key);
				return false;
			}
			return true;
		};

		if (section == Section::Root) {
			if (key == "encounter_id") {
				parseUIntField(def.encounter.encounterId);
			} else if (key == "start_phase_id") {
				uint32_t tmp = 0;
				if (parseUIntField(tmp))
					def.encounter.startPhaseId = static_cast<uint8_t>(tmp);
			} else if (key == "enrage_time_ms") {
				parseUIntField(def.encounter.enrageTimeMs);
			}
			continue;
		}

		if (section == Section::Phase && !def.encounter.phases.empty()) {
			EncounterPhaseDef &phase = def.encounter.phases.back();
			if (key == "id") {
				uint32_t tmp = 0;
				if (parseUIntField(tmp))
					phase.id = static_cast<uint8_t>(tmp);
			} else if (key == "failure_policy") {
				auto policy = ParseFailurePolicy(value);
				if (!policy)
					AddError(result.errors, lineNo, "Unknown failure_policy: " + value);
				else
					phase.failurePolicy = *policy;
			} else if (key == "exit_condition") {
				EncounterCondition cond;
				if (!ParseConditionText(value, cond))
					AddError(result.errors, lineNo, "Invalid exit_condition format");
				else
					phase.exitCondition = cond;
			} else if (key == "next_phase_id") {
				uint32_t tmp = 0;
				if (parseUIntField(tmp))
					phase.nextPhaseId = static_cast<uint8_t>(tmp);
			}
			continue;
		}

		if (section == Section::Mechanic && !def.encounter.mechanics.empty()) {
			EncounterMechanic &mechanic = def.encounter.mechanics.back();
			if (key == "id") {
				uint32_t tmp = 0;
				if (parseUIntField(tmp))
					mechanic.id = static_cast<uint16_t>(tmp);
			} else if (key == "type") {
				auto type = ParseMechanicType(value);
				if (!type)
					AddError(result.errors, lineNo, "Unknown mechanic type: " + value);
				else
					mechanic.type = *type;
			} else if (key == "phase_id") {
				uint32_t tmp = 0;
				if (parseUIntField(tmp))
					mechanic.phaseId = static_cast<uint8_t>(tmp);
			} else if (key == "period_ms") {
				parseUIntField(mechanic.periodMs);
			} else if (key == "first_trigger_ms") {
				parseUIntField(mechanic.firstTriggerMs);
			} else if (key == "repeat") {
				mechanic.repeat = (value == "true");
			} else if (key == "trigger_condition") {
				EncounterCondition cond;
				if (!ParseConditionText(value, cond))
					AddError(result.errors, lineNo, "Invalid trigger_condition format");
				else
					mechanic.triggerCondition = cond;
			}
			continue;
		}

		if (section == Section::Reward && !def.rewards.empty()) {
			EncounterRewardDef &reward = def.rewards.back();
			if (key == "id") {
				reward.rewardId = value;
			} else if (key == "quantity") {
				parseUIntField(reward.quantity);
			}
		}
	}

	result.definition = def;
	auto validationErrors = ValidateEncounterSchema(def);
	result.errors.insert(result.errors.end(), validationErrors.begin(), validationErrors.end());
	if (!result.errors.empty())
		result.definition.reset();
	return result;
}

std::vector<EncounterSchemaError> ValidateEncounterSchema(const EncounterSchemaDefinition &definition)
{
	std::vector<EncounterSchemaError> errors;
	std::unordered_set<uint8_t> phaseIds;
	for (const EncounterPhaseDef &phase : definition.encounter.phases) {
		if (!phaseIds.insert(phase.id).second)
			AddError(errors, 0, "Duplicate phase id: " + std::to_string(phase.id));
	}
	if (phaseIds.find(definition.encounter.startPhaseId) == phaseIds.end())
		AddError(errors, 0, "start_phase_id does not exist in phases");

	std::unordered_set<uint16_t> mechanicIds;
	for (const EncounterMechanic &mechanic : definition.encounter.mechanics) {
		if (!mechanicIds.insert(mechanic.id).second)
			AddError(errors, 0, "Duplicate mechanic id: " + std::to_string(mechanic.id));
		if (phaseIds.find(mechanic.phaseId) == phaseIds.end())
			AddError(errors, 0, "mechanic references missing phase id: " + std::to_string(mechanic.phaseId));
	}

	for (const EncounterPhaseDef &phase : definition.encounter.phases) {
		if (phase.nextPhaseId && phaseIds.find(*phase.nextPhaseId) == phaseIds.end())
			AddError(errors, 0, "phase references missing next_phase_id: " + std::to_string(*phase.nextPhaseId));
	}

	for (const EncounterRewardDef &reward : definition.rewards) {
		if (reward.rewardId.empty())
			AddError(errors, 0, "reward id must not be empty");
		if (reward.quantity == 0)
			AddError(errors, 0, "reward quantity must be > 0 for reward: " + reward.rewardId);
	}
	return errors;
}

} // namespace devilution
