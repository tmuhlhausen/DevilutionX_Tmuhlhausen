#include "engine/render/render_graph.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace devilution {

TEST(RenderGraphTest, ExecutesDefaultThreePassOrder)
{
	std::vector<RenderPassId> executed;
	RenderGraph graph = CreateDefaultRenderGraph(
	    [&] { executed.push_back(RenderPassId::World); },
	    [&] { executed.push_back(RenderPassId::Ui); },
	    [&] { executed.push_back(RenderPassId::Post); });

	ASSERT_TRUE(graph.Execute());
	ASSERT_EQ(executed.size(), 3u);
	EXPECT_EQ(executed[0], RenderPassId::World);
	EXPECT_EQ(executed[1], RenderPassId::Ui);
	EXPECT_EQ(executed[2], RenderPassId::Post);
}

TEST(RenderGraphTest, DetectsCycles)
{
	RenderGraph graph;
	graph.AddPass({ RenderPassId::World, "world", { RenderPassId::Post }, [] {} });
	graph.AddPass({ RenderPassId::Ui, "ui", { RenderPassId::World }, [] {} });
	graph.AddPass({ RenderPassId::Post, "post", { RenderPassId::Ui }, [] {} });

	EXPECT_FALSE(graph.Execute());
}

TEST(RenderGraphTest, FailsValidationWhenDependencyNodeMissing)
{
	RenderGraph graph;
	graph.AddPass({ RenderPassId::World, "world", {}, [] {} });
	graph.AddPass({ RenderPassId::Ui, "ui", { RenderPassId::World }, [] {} });
	graph.AddPass({ RenderPassId::Post, "post", { RenderPassId::Ui, RenderPassId::World }, [] {} });

	EXPECT_TRUE(graph.Validate());

	RenderGraph brokenGraph;
	brokenGraph.AddPass({ RenderPassId::World, "world", {}, [] {} });
	brokenGraph.AddPass({ RenderPassId::Post, "post", { RenderPassId::Ui }, [] {} });

	EXPECT_FALSE(brokenGraph.Validate());
	EXPECT_FALSE(brokenGraph.Execute());
}

TEST(RenderGraphTest, MissingMigrationNodePreventsPassExecution)
{
	std::vector<RenderPassId> executed;

	RenderGraph migratingGraph;
	migratingGraph.AddPass({ RenderPassId::World, "world", {}, [&] { executed.push_back(RenderPassId::World); } });
	migratingGraph.AddPass({ RenderPassId::Post, "post", { RenderPassId::Ui }, [&] { executed.push_back(RenderPassId::Post); } });

	EXPECT_FALSE(migratingGraph.Validate());
	EXPECT_FALSE(migratingGraph.Execute());
	EXPECT_TRUE(executed.empty());
}

TEST(RenderGraphTest, MigrationValidationRequiresDeclaredDependencies)
{
	RenderGraph migratingGraph;
	migratingGraph.AddPass({ RenderPassId::World, "world", {}, [] {} });
	migratingGraph.AddPass({ RenderPassId::Ui, "ui", { RenderPassId::World }, [] {} });
	migratingGraph.AddPass({ RenderPassId::Post, "post", { RenderPassId::Ui, RenderPassId::World }, [] {} });

	EXPECT_TRUE(migratingGraph.Validate());

	RenderGraph missingWorldDependencyGraph;
	missingWorldDependencyGraph.AddPass({ RenderPassId::Ui, "ui", { RenderPassId::World }, [] {} });
	missingWorldDependencyGraph.AddPass({ RenderPassId::Post, "post", { RenderPassId::Ui, RenderPassId::World }, [] {} });

	EXPECT_FALSE(missingWorldDependencyGraph.Validate());
}

TEST(RenderGraphTest, FailsValidationWhenPassIdDuplicated)
{
	RenderGraph graph;
	graph.AddPass({ RenderPassId::World, "world", {}, [] {} });
	graph.AddPass({ RenderPassId::World, "world_shadow", {}, [] {} });
	graph.AddPass({ RenderPassId::Ui, "ui", { RenderPassId::World }, [] {} });

	EXPECT_FALSE(graph.Validate());
	EXPECT_FALSE(graph.Execute());
}

} // namespace devilution
