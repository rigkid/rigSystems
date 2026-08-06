#include "SHierarchy.h"
#include "CRelationship.h"
#include "CTransform.h"

#include <unordered_set>

namespace rigkit {
namespace ecs {
namespace {

constexpr int kMaxParentDepth = 64;

// Kept across frames: clear() drops entries but keeps bucket capacity, so steady
// scenes do not allocate on the Update hot path every tick.
std::unordered_set<entt::entity> g_done;
std::unordered_set<entt::entity> g_stack;

/**
 * @brief State of one SHierarchy walk: the world plus what it has visited.
 * @details Lives for a single pass. Visit sets are file-static capacity reuse —
 * not a retained ECS owner.
 */
struct WorldPass {
	MEcs& ecs;
	std::unordered_set<entt::entity>& done;
	std::unordered_set<entt::entity>& stack;

	void compute(entt::entity entity, int depth);
};

void WorldPass::compute(entt::entity entity, int depth) {
	if (done.count(entity)) {
		return;
	}
	if (!ecs.hasComponent<CTransform>(entity)) {
		done.insert(entity);
		return;
	}

	auto& transform = ecs.getComponent<CTransform>(entity);
	const glm::mat4 local = transform.localMatrix();

	entt::entity parent = entt::null;
	if (ecs.hasComponent<CRelationship>(entity)) {
		parent = ecs.getComponent<CRelationship>(entity).parent;
	}

	if (parent == entt::null || depth >= kMaxParentDepth || !ecs.hasComponent<CTransform>(parent) ||
		stack.count(parent)) {
		transform.world = local;
		done.insert(entity);
		return;
	}

	stack.insert(entity);
	compute(parent, depth + 1);
	stack.erase(entity);

	transform.world = ecs.getComponent<CTransform>(parent).world * local;
	done.insert(entity);
}

} // namespace

void SHierarchy(MEcs& ecs) {
	g_done.clear();
	g_stack.clear();
	WorldPass pass{ecs, g_done, g_stack};
	for (auto entity : ecs.view<CTransform>()) {
		pass.compute(entity, 0);
	}
}

} // namespace ecs
} // namespace rigkit
