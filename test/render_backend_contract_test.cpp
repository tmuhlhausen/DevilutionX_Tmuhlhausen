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

} // namespace devilution
