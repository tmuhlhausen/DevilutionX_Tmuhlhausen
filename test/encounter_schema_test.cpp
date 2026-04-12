#include <gtest/gtest.h>

#include "raid/encounter/encounter_schema.hpp"

namespace devilution {
namespace {

TEST(EncounterSchemaTest, ValidTomlParsesAndValidates)
{
	constexpr std::string_view Toml = R"(
encounter_id = 42
start_phase_id = 0

[[phases]]
id = 0
failure_policy = "Wipe"
exit_condition = "TimeElapsedAtLeast:100"
next_phase_id = 2

[[phases]]
id = 2
failure_policy = "CheckpointRollback"

[[mechanics]]
id = 10
type = "AoEPulse"
phase_id = 0
period_ms = 50
repeat = true
trigger_condition = "BossHealthAtMost:80"

[[rewards]]
id = "token"
quantity = 1
)";

	const EncounterSchemaLoadResult result = LoadEncounterSchema(Toml, EncounterSchemaFormat::Toml);
	ASSERT_TRUE(result.errors.empty());
	ASSERT_TRUE(result.definition.has_value());
	EXPECT_EQ(result.definition->encounter.phases.size(), 2u);
	EXPECT_EQ(result.definition->encounter.mechanics.size(), 1u);
}

TEST(EncounterSchemaTest, InvalidReferencesAreRejected)
{
	EncounterSchemaDefinition def;
	def.encounter.encounterId = 7;
	def.encounter.startPhaseId = 9;
	def.encounter.phases.push_back(EncounterPhaseDef { .id = 0 });
	def.encounter.mechanics.push_back(EncounterMechanic { .id = 1, .phaseId = 8 });
	def.rewards.push_back(EncounterRewardDef { .rewardId = "", .quantity = 0 });

	const auto errors = ValidateEncounterSchema(def);
	EXPECT_FALSE(errors.empty());
}

TEST(EncounterSchemaTest, LootTableRejectsEmptyIdAndZeroQuantity)
{
	constexpr std::string_view Toml = R"(
encounter_id = 101
start_phase_id = 0

[[phases]]
id = 0

[[rewards]]
id = ""
quantity = 0
)";

	const EncounterSchemaLoadResult result = LoadEncounterSchema(Toml, EncounterSchemaFormat::Toml);
	EXPECT_FALSE(result.errors.empty());
	EXPECT_FALSE(result.definition.has_value());
}

TEST(EncounterSchemaTest, LootTableAcceptsMultipleDistinctRewards)
{
	constexpr std::string_view Toml = R"(
encounter_id = 102
start_phase_id = 0

[[phases]]
id = 0

[[rewards]]
id = "token"
quantity = 2

[[rewards]]
id = "gem"
quantity = 1
)";

	const EncounterSchemaLoadResult result = LoadEncounterSchema(Toml, EncounterSchemaFormat::Toml);
	ASSERT_TRUE(result.errors.empty());
	ASSERT_TRUE(result.definition.has_value());
	ASSERT_EQ(result.definition->rewards.size(), 2u);
	EXPECT_EQ(result.definition->rewards[0].rewardId, "token");
	EXPECT_EQ(result.definition->rewards[0].quantity, 2u);
	EXPECT_EQ(result.definition->rewards[1].rewardId, "gem");
	EXPECT_EQ(result.definition->rewards[1].quantity, 1u);
}

} // namespace
} // namespace devilution
