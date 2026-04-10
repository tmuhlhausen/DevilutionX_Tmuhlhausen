#pragma once

#include <cstdint>
#include <string_view>

namespace devilution {

struct RenderFrameStats {
	uint32_t drawCalls;
	uint32_t triangles;
	float gpuFrameMs;
};

class IRenderBackend {
public:
	virtual ~IRenderBackend() = default;

	virtual std::string_view Name() const = 0;
	virtual bool Initialize() = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual RenderFrameStats GetLastFrameStats() const = 0;
};

} // namespace devilution
