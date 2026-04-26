#include "engine/render/render_backend.hpp"

#include <type_traits>

#include <gtest/gtest.h>

namespace devilution {
namespace {

class NullRenderBackend final : public IRenderBackend {
public:
	std::string_view Name() const override
	{
		return "null";
	}

	bool Initialize() override
	{
		return true;
	}

	void BeginFrame() override {}
	void EndFrame() override {}

	RenderFrameStats GetLastFrameStats() const override
	{
		return RenderFrameStats { 1, 2, 3.0F };
	}
};

} // namespace

TEST(RenderBackendContractTest, ExposesBackendAgnosticContract)
{
	static_assert(std::is_polymorphic_v<IRenderBackend>);
	NullRenderBackend backend;
	SetActiveRenderBackend(&backend);
	ASSERT_EQ(GetActiveRenderBackend(), &backend);
	EXPECT_EQ(GetActiveRenderBackend()->Name(), "null");
	EXPECT_TRUE(GetActiveRenderBackend()->Initialize());
	const RenderFrameStats stats = GetActiveRenderBackend()->GetLastFrameStats();
	EXPECT_EQ(stats.drawCalls, 1u);
	EXPECT_EQ(stats.triangles, 2u);
	EXPECT_FLOAT_EQ(stats.gpuFrameMs, 3.0F);
}

TEST(RenderBackendContractTest, StoresRenderGraphPassStats)
{
	const RenderGraphPassStats expected = {
		RenderFrameStats { 11, 12, 1.1F },
		RenderFrameStats { 21, 22, 2.2F },
		RenderFrameStats { 31, 32, 3.3F },
	};

	SetLastRenderGraphPassStats(expected);

	const RenderGraphPassStats &captured = GetLastRenderGraphPassStats();
	EXPECT_EQ(captured[0].drawCalls, expected[0].drawCalls);
	EXPECT_EQ(captured[0].triangles, expected[0].triangles);
	EXPECT_FLOAT_EQ(captured[0].gpuFrameMs, expected[0].gpuFrameMs);
	EXPECT_EQ(captured[1].drawCalls, expected[1].drawCalls);
	EXPECT_EQ(captured[1].triangles, expected[1].triangles);
	EXPECT_FLOAT_EQ(captured[1].gpuFrameMs, expected[1].gpuFrameMs);
	EXPECT_EQ(captured[2].drawCalls, expected[2].drawCalls);
	EXPECT_EQ(captured[2].triangles, expected[2].triangles);
	EXPECT_FLOAT_EQ(captured[2].gpuFrameMs, expected[2].gpuFrameMs);
}

} // namespace devilution
