#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace devilution {

struct RenderFrameStats {
	uint32_t drawCalls;
	uint32_t triangles;
	float gpuFrameMs;
};

constexpr size_t RenderGraphPassCount = 3;
using RenderGraphPassStats = std::array<RenderFrameStats, RenderGraphPassCount>;

class IRenderBackend {
public:
	virtual ~IRenderBackend() = default;

	virtual std::string_view Name() const = 0;
	virtual bool Initialize() = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual RenderFrameStats GetLastFrameStats() const = 0;
};

void SetActiveRenderBackend(IRenderBackend *backend);
[[nodiscard]] IRenderBackend *GetActiveRenderBackend();

void SetLastRenderGraphPassStats(const RenderGraphPassStats &passStats);
[[nodiscard]] const RenderGraphPassStats &GetLastRenderGraphPassStats();

} // namespace devilution
