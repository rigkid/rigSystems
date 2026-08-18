#include "SOrbitDrive.h"

#include "COrbitDrive.h"
#include "CTransform.h"
#include "rig/create.h"

#include <algorithm>
#include <cmath>

namespace rigkit {
namespace ecs {

void SOrbitDrive(MEcs& ecs, float dt) {
	for (auto e : ecs.view<ecs::COrbitDrive, ecs::CTransform>()) {
		auto& orbit = ecs.getComponent<ecs::COrbitDrive>(e);
		if (!orbit.enabled) {
			continue;
		}
		orbit.speed = std::max(0.f, orbit.speed);
		orbit.radius = std::max(0.01f, orbit.radius);
		orbit.yaw += dt * orbit.speed;
		const glm::vec3 eye{std::sin(orbit.yaw) * orbit.radius, orbit.height,
							std::cos(orbit.yaw) * orbit.radius};
		rig::lookAt(ecs.getComponent<ecs::CTransform>(e), eye, orbit.target);
	}
}

} // namespace ecs
} // namespace rigkit
