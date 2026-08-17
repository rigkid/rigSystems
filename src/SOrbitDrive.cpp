#include "SOrbitDrive.h"

#include "COrbitDrive.h"
#include "CTransform.h"
#include "rig/create.h"

#include <algorithm>
#include <cmath>

namespace rigkit {
namespace ecs {
namespace {

constexpr float kPitchLimit = 1.4f;

glm::vec3 orbitEye(const COrbitDrive& orbit) {
	const float cp = std::cos(orbit.pitch);
	const float sp = std::sin(orbit.pitch);
	const float sy = std::sin(orbit.yaw);
	const float cy = std::cos(orbit.yaw);
	const float r = orbit.radius;
	return orbit.target + glm::vec3{sy * cp * r, sp * r, cy * cp * r};
}

} // namespace

void SOrbitDrive(MEcs& ecs, float dt) {
	for (auto e : ecs.view<ecs::COrbitDrive, ecs::CTransform>()) {
		auto& orbit = ecs.getComponent<ecs::COrbitDrive>(e);
		if (!orbit.enabled) {
			continue;
		}
		orbit.speed = std::max(0.f, orbit.speed);
		orbit.radius = std::max(0.01f, orbit.radius);
		orbit.pitch = std::clamp(orbit.pitch, -kPitchLimit, kPitchLimit);
		orbit.yaw += dt * orbit.speed;
		rig::lookAt(ecs.getComponent<ecs::CTransform>(e), orbitEye(orbit), orbit.target);
	}
}

} // namespace ecs
} // namespace rigkit
