#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

namespace devilution {

enum class RenderPassId : uint8_t {
	World,
	Ui,
	Post,
};

struct RenderPassNode {
	RenderPassId id;
	std::string_view name;
	std::vector<RenderPassId> dependsOn;
	std::function<void()> execute;
};

class RenderGraph {
public:
	void AddPass(RenderPassNode pass)
	{
		passes_.push_back(std::move(pass));
	}

	[[nodiscard]] bool Execute()
	{
		orderedPasses_.clear();
		std::array<uint8_t, 3> state {}; // 0=unvisited, 1=visiting, 2=visited
		for (const RenderPassNode &pass : passes_) {
			if (!Visit(pass.id, state))
				return false;
		}

		for (RenderPassId id : orderedPasses_) {
			RenderPassNode *pass = FindPass(id);
			if (pass != nullptr && pass->execute)
				pass->execute();
		}
		return true;
	}

	[[nodiscard]] const std::vector<RenderPassId> &GetExecutionOrder() const
	{
		return orderedPasses_;
	}

private:
	[[nodiscard]] static constexpr size_t ToIndex(RenderPassId id)
	{
		return static_cast<size_t>(id);
	}

	[[nodiscard]] RenderPassNode *FindPass(RenderPassId id)
	{
		for (RenderPassNode &pass : passes_) {
			if (pass.id == id)
				return &pass;
		}
		return nullptr;
	}

	[[nodiscard]] const RenderPassNode *FindPass(RenderPassId id) const
	{
		for (const RenderPassNode &pass : passes_) {
			if (pass.id == id)
				return &pass;
		}
		return nullptr;
	}

	[[nodiscard]] bool Visit(RenderPassId id, std::array<uint8_t, 3> &state)
	{
		const size_t index = ToIndex(id);
		if (state[index] == 2)
			return true;
		if (state[index] == 1)
			return false;
		const RenderPassNode *pass = FindPass(id);
		if (pass == nullptr)
			return false;
		state[index] = 1;
		for (RenderPassId dependency : pass->dependsOn) {
			if (!Visit(dependency, state))
				return false;
		}
		state[index] = 2;
		orderedPasses_.push_back(id);
		return true;
	}

	std::vector<RenderPassNode> passes_;
	std::vector<RenderPassId> orderedPasses_;
};

inline RenderGraph CreateDefaultRenderGraph(std::function<void()> worldPass, std::function<void()> uiPass, std::function<void()> postPass)
{
	RenderGraph graph;
	graph.AddPass({ RenderPassId::World, "world", {}, std::move(worldPass) });
	graph.AddPass({ RenderPassId::Ui, "ui", { RenderPassId::World }, std::move(uiPass) });
	graph.AddPass({ RenderPassId::Post, "post", { RenderPassId::Ui }, std::move(postPass) });
	return graph;
}

} // namespace devilution
