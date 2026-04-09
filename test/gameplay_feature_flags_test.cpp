#include <gtest/gtest.h>

#include "options.h"

namespace devilution {

TEST(GameplayFeatureFlags, DefaultsAreSafeAndStaged)
{
	EXPECT_FALSE(GetOptions().Gameplay.phaseANephilimCore);
	EXPECT_FALSE(GetOptions().Gameplay.phaseBRarityDropPipeline);
	EXPECT_FALSE(GetOptions().Gameplay.phaseCGuildCoreProtocol);
	EXPECT_FALSE(GetOptions().Gameplay.phaseDGuildHallsEndgame);
	EXPECT_FALSE(GetOptions().Gameplay.phaseEBalanceAntiCheat);
	EXPECT_TRUE(GetOptions().Gameplay.requireDeterministicMultiplayerItemRecreation);
}

TEST(GameplayFeatureFlags, IndependentToggleControl)
{
	GetOptions().Gameplay.phaseANephilimCore.SetValue(true);
	GetOptions().Gameplay.phaseBRarityDropPipeline.SetValue(false);
	GetOptions().Gameplay.phaseCGuildCoreProtocol.SetValue(true);
	GetOptions().Gameplay.phaseDGuildHallsEndgame.SetValue(false);
	GetOptions().Gameplay.phaseEBalanceAntiCheat.SetValue(true);

	EXPECT_TRUE(GetOptions().Gameplay.phaseANephilimCore);
	EXPECT_FALSE(GetOptions().Gameplay.phaseBRarityDropPipeline);
	EXPECT_TRUE(GetOptions().Gameplay.phaseCGuildCoreProtocol);
	EXPECT_FALSE(GetOptions().Gameplay.phaseDGuildHallsEndgame);
	EXPECT_TRUE(GetOptions().Gameplay.phaseEBalanceAntiCheat);
}

} // namespace devilution
