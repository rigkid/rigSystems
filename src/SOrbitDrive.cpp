#include "SOrbitDrive.h"

#include "COrbitDrive.h"
#include "CTransform.h"
#include "OrbitNav.h"

#include <algorithm>

namespace rigkit {
namespace ecs {

void SOrbitDrive(MEcs& ecs, float dt) {
	for (auto e : ecs.view<ecs::COrbitDrive, ecs::CTransform>()) {
		auto& orbit = ecs.getComponent<ecs::COrbitDrive>(e);
		if (!orbit.enabled) {
			continue;
		}
		orbit.speed = std::max(0.f, orbit.speed);
		orbit.yaw += dt * orbit.speed;
		rig::orbitApply(ecs, e);
	}
}

} // namespace ecs
} // namespace rigkit
