#include <gtest/gtest.h>

#include "lua/lua_global.hpp"

namespace devilution {
namespace {

TEST(LuaPackageNameTest, AcceptsSimpleAndNestedPackageNames)
{
	EXPECT_TRUE(IsValidLuaPackageName("inspect"));
	EXPECT_TRUE(IsValidLuaPackageName("devilutionx.events"));
	EXPECT_TRUE(IsValidLuaPackageName("mods.My_Mod_01.init"));
}

TEST(LuaPackageNameTest, RejectsEmptyOrEmptySegments)
{
	EXPECT_FALSE(IsValidLuaPackageName(""));
	EXPECT_FALSE(IsValidLuaPackageName("."));
	EXPECT_FALSE(IsValidLuaPackageName(".mods.init"));
	EXPECT_FALSE(IsValidLuaPackageName("mods..init"));
	EXPECT_FALSE(IsValidLuaPackageName("mods.init."));
}

TEST(LuaPackageNameTest, RejectsPathSeparatorsAndTraversal)
{
	EXPECT_FALSE(IsValidLuaPackageName("../mods/init"));
	EXPECT_FALSE(IsValidLuaPackageName("mods/evil/init"));
	EXPECT_FALSE(IsValidLuaPackageName("mods\\evil\\init"));
	EXPECT_FALSE(IsValidLuaPackageName("mods...evil"));
}

TEST(LuaPackageNameTest, RejectsUnsupportedCharacters)
{
	EXPECT_FALSE(IsValidLuaPackageName("mods.my-mod.init"));
	EXPECT_FALSE(IsValidLuaPackageName("mods.my mod.init"));
	EXPECT_FALSE(IsValidLuaPackageName("mods.my:mod.init"));
	EXPECT_FALSE(IsValidLuaPackageName("mods.my@mod.init"));
}

} // namespace
} // namespace devilution
