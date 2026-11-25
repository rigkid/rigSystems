#include "rendering/TextureYOrigin.h"
#include "COrbitDrive.h"
#include "OrbitNav.h"

#include <cmath>
#include <cstdio>

int main() {
	using rigkit::TextureYOrigin;
	using rigkit::ecs::COrbitDrive;

	const glm::vec3 right{1.f, 0.f, 0.f};
	const glm::vec3 up{0.f, 1.f, 0.f};
	const auto bl = rig::orbitTruckDelta(TextureYOrigin::BottomLeft, right, up, 10.f, 5.f, 1.f);
	const auto tl = rig::orbitTruckDelta(TextureYOrigin::TopLeft, right, up, 10.f, 5.f, 1.f);
	const bool truckBl = std::abs(bl.x + 10.f) < 1e-4f && std::abs(bl.y - 5.f) < 1e-4f;
	const bool truckTl = std::abs(tl.x + 10.f) < 1e-4f && std::abs(tl.y + 5.f) < 1e-4f;

	COrbitDrive yUp;
	yUp.worldUp = COrbitDrive::WorldUp::Y;
	yUp.target = {0.f, 0.f, 0.f};
	yUp.radius = 10.f;
	yUp.yaw = 0.f;
	yUp.pitch = 0.f;
	const glm::vec3 eyeY = rig::orbitEye(yUp);
	const bool yEye =
		std::abs(eyeY.x) < 1e-4f && std::abs(eyeY.y) < 1e-4f && std::abs(eyeY.z - 10.f) < 1e-4f;

	COrbitDrive zUp;
	zUp.worldUp = COrbitDrive::WorldUp::Z;
	zUp.target = {0.f, 0.f, 0.f};
	zUp.radius = 10.f;
	zUp.yaw = 0.f;
	zUp.pitch = 0.f;
	const glm::vec3 eyeZ = rig::orbitEye(zUp);
	// Host Space Front: yaw 0 / pitch 0 sits on -Y.
	const bool zEye =
		std::abs(eyeZ.x) < 1e-4f && std::abs(eyeZ.y + 10.f) < 1e-4f && std::abs(eyeZ.z) < 1e-4f;

	zUp.pitch = 1.5707963f - 0.02f;
	const glm::vec3 eyeTop = rig::orbitEye(zUp);
	const bool zTop = eyeTop.z > 9.f && std::abs(eyeTop.x) < 1.f && std::abs(eyeTop.y) < 1.f;
	// Top must survive clampOrbit / orbitApply (limit >= pi/2 - 0.02).
	const bool topUnclamped = zUp.pitch <= rig::kOrbitPitchLimit + 1e-6f;

	const bool ndc =
		std::abs(rigkit::presentNdcYFromWidgetLocal(TextureYOrigin::BottomLeft, 0.f, 100.f) - 1.f) <
		1e-6f;

	const bool ok = truckBl && truckTl && yEye && zEye && zTop && topUnclamped && ndc;
	std::printf(
		"orbit_nav_smoke truck_bl=%d truck_tl=%d y_eye=%d z_front=%d z_top=%d top_ok=%d ndc=%d\n",
		truckBl ? 1 : 0, truckTl ? 1 : 0, yEye ? 1 : 0, zEye ? 1 : 0, zTop ? 1 : 0,
		topUnclamped ? 1 : 0, ndc ? 1 : 0);
	return ok ? 0 : 1;
}
