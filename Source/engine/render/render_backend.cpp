#include "engine/render/render_backend.hpp"

namespace devilution {
namespace {

IRenderBackend *ActiveRenderBackend = nullptr;

} // namespace

void SetActiveRenderBackend(IRenderBackend *backend)
{
	ActiveRenderBackend = backend;
}

IRenderBackend *GetActiveRenderBackend()
{
	return ActiveRenderBackend;
}

} // namespace devilution
