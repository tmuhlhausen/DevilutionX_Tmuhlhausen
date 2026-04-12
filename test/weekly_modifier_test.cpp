#include <gtest/gtest.h>

#include "raid/encounter/weekly_modifiers.hpp"

namespace devilution {
namespace {

TEST(WeeklyModifierTest, RotationIsDeterministicAcrossHostAndClient)
{
	WeeklyModifierRegistry host;
	WeeklyModifierRegistry client;
	host.RegisterDefaults();
	client.RegisterDefaults();

	const auto hostRotation = host.GetWeeklyRotation(3, 14, 3);
	const auto clientRotation = client.GetWeeklyRotation(3, 14, 3);
	EXPECT_EQ(hostRotation, clientRotation);
}

TEST(WeeklyModifierTest, RotationChangesWithWeek)
{
	WeeklyModifierRegistry registry;
	registry.RegisterDefaults();

	const auto weekA = registry.GetWeeklyRotation(1, 1, 3);
	const auto weekB = registry.GetWeeklyRotation(1, 2, 3);
	EXPECT_NE(weekA, weekB);
}

} // namespace
} // namespace devilution
