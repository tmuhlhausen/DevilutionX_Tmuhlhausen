#include "engine/render/render_backend.hpp"

namespace devilution {
namespace {

IRenderBackend *ActiveRenderBackend = nullptr;
RenderGraphPassStats LastRenderGraphPassStats {};

} // namespace

void SetActiveRenderBackend(IRenderBackend *backend)
{
	ActiveRenderBackend = backend;
}

IRenderBackend *GetActiveRenderBackend()
{
	return ActiveRenderBackend;
}

void SetLastRenderGraphPassStats(const RenderGraphPassStats &passStats)
{
	LastRenderGraphPassStats = passStats;
}

const RenderGraphPassStats &GetLastRenderGraphPassStats()
{
	return LastRenderGraphPassStats;
}

} // namespace devilution
