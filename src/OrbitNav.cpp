#include "OrbitNav.h"

#include <glm/gtc/matrix_inverse.hpp>

#include "CMesh.h"
#include "COrbitDrive.h"
#include "CSelection.h"
#include "CTransform.h"
#include "SHierarchy.h"
#include "rig/create.h"

#include <algorithm>
#include <cmath>

namespace rig {
namespace {

glm::vec3 orbitEye(const rigkit::ecs::COrbitDrive& orbit) {
	const float cp = std::cos(orbit.pitch);
	const float sp = std::sin(orbit.pitch);
	const float sy = std::sin(orbit.yaw);
	const float cy = std::cos(orbit.yaw);
	const float r = orbit.radius;
	return orbit.target + glm::vec3{sy * cp * r, sp * r, cy * cp * r};
}

void clampOrbit(rigkit::ecs::COrbitDrive& orbit) {
	orbit.pitch = std::clamp(orbit.pitch, -kOrbitPitchLimit, kOrbitPitchLimit);
	orbit.radius = std::clamp(orbit.radius, kOrbitRadiusMin, kOrbitRadiusMax);
	orbit.height = std::sin(orbit.pitch) * orbit.radius;
}

bool meshWorldBounds(rigkit::MEcs& ecs, entt::entity e, glm::vec3& bmin, glm::vec3& bmax) {
	if (!ecs.hasComponent<rigkit::ecs::CMesh>(e) || !ecs.hasComponent<rigkit::ecs::CTransform>(e)) {
		return false;
	}
	const auto& mesh = ecs.getComponent<rigkit::ecs::CMesh>(e);
	if (mesh.mode != rigkit::ecs::CMesh::Mode::Triangles || mesh.positions.empty()) {
		return false;
	}
	const glm::mat4& world = ecs.getComponent<rigkit::ecs::CTransform>(e).world;
	for (const auto& p : mesh.positions) {
		const glm::vec3 w = glm::vec3(world * glm::vec4(p, 1.f));
		bmin = glm::min(bmin, w);
		bmax = glm::max(bmax, w);
	}
	return true;
}

} // namespace

void orbitApply(rigkit::MEcs& ecs, entt::entity cam) {
	if (cam == entt::null || !ecs.hasComponent<rigkit::ecs::COrbitDrive>(cam) ||
		!ecs.hasComponent<rigkit::ecs::CTransform>(cam)) {
		return;
	}
	auto& orbit = ecs.getComponent<rigkit::ecs::COrbitDrive>(cam);
	clampOrbit(orbit);
	lookAt(ecs.getComponent<rigkit::ecs::CTransform>(cam), orbitEye(orbit), orbit.target);
}

void orbitNavigate(rigkit::MEcs& ecs, entt::entity cam, const OrbitNavFrame& frame,
				   OrbitNavState& state) {
	if (cam == entt::null || !ecs.hasComponent<rigkit::ecs::COrbitDrive>(cam) ||
		!ecs.hasComponent<rigkit::ecs::CTransform>(cam)) {
		state.dragging = false;
		return;
	}
	if (frame.blocked) {
		state.dragging = false;
		return;
	}

	auto& orbit = ecs.getComponent<rigkit::ecs::COrbitDrive>(cam);
	const bool drag = frame.orbit || frame.pan || frame.dolly;
	if (drag) {
		if (!state.dragging) {
			state.dragging = true;
			state.lastX = frame.mouseX;
			state.lastY = frame.mouseY;
			orbit.enabled = false;
		} else {
			const float dx = frame.mouseX - state.lastX;
			const float dy = frame.mouseY - state.lastY;
			state.lastX = frame.mouseX;
			state.lastY = frame.mouseY;
			if (frame.dolly) {
				orbit.radius *= (dy > 0.f ? 1.01f : 0.99f);
			} else if (frame.pan) {
				const float cp = std::cos(orbit.pitch);
				const float sp = std::sin(orbit.pitch);
				const float sy = std::sin(orbit.yaw);
				const float cy = std::cos(orbit.yaw);
				const glm::vec3 right{cy, 0.f, -sy};
				const glm::vec3 up{-sy * sp, cp, -cy * sp};
				orbit.target += (-dx * right + dy * up) * (orbit.radius * 0.0025f);
			} else {
				orbit.yaw -= dx * 0.005f;
				orbit.pitch += dy * 0.005f;
			}
			orbitApply(ecs, cam);
		}
	} else {
		state.dragging = false;
	}

	if (std::fabs(frame.wheel) > 1e-4f) {
		orbit.enabled = false;
		orbit.radius *= (frame.wheel > 0.f ? 0.9f : 1.1f);
		orbitApply(ecs, cam);
	}
}

bool orbitFrame(rigkit::MEcs& ecs, entt::entity cam, const glm::vec3& bmin, const glm::vec3& bmax) {
	if (cam == entt::null || !ecs.hasComponent<rigkit::ecs::COrbitDrive>(cam)) {
		return false;
	}
	const glm::vec3 extent = bmax - bmin;
	if (extent.x < 0.f || extent.y < 0.f || extent.z < 0.f) {
		return false;
	}
	auto& orbit = ecs.getComponent<rigkit::ecs::COrbitDrive>(cam);
	orbit.target = (bmin + bmax) * 0.5f;
	orbit.radius = std::max(glm::length(extent) * 1.1f, 2.5f);
	orbit.enabled = false;
	orbitApply(ecs, cam);
	return true;
}

bool orbitFrameMeshes(rigkit::MEcs& ecs, entt::entity cam) {
	rigkit::ecs::SHierarchy(ecs);
	glm::vec3 bmin(1e9f);
	glm::vec3 bmax(-1e9f);
	bool any = false;

	for (auto e : ecs.view<rigkit::ecs::CSelection>()) {
		const auto& sel = ecs.getComponent<rigkit::ecs::CSelection>(e);
		if (!sel.isSelected && !sel.isMultiSelected) {
			continue;
		}
		if (meshWorldBounds(ecs, e, bmin, bmax)) {
			any = true;
		}
	}
	if (!any) {
		bmin = glm::vec3(1e9f);
		bmax = glm::vec3(-1e9f);
		for (auto e : ecs.view<rigkit::ecs::CMesh, rigkit::ecs::CTransform>()) {
			if (meshWorldBounds(ecs, e, bmin, bmax)) {
				any = true;
			}
		}
	}
	return any && orbitFrame(ecs, cam, bmin, bmax);
}

void orbitFromView(rigkit::ecs::COrbitDrive& orbit, const glm::mat4& view,
				   rigkit::ecs::CTransform& camXf) {
	const glm::vec3 eye = glm::vec3(glm::inverse(view)[3]);
	const glm::vec3 off = eye - orbit.target;
	const float r = glm::length(off);
	if (r < 1e-3f) {
		return;
	}
	orbit.radius = r;
	orbit.yaw = std::atan2(off.x, off.z);
	orbit.pitch = std::asin(std::clamp(off.y / r, -1.f, 1.f));
	orbit.enabled = false;
	clampOrbit(orbit);
	lookAt(camXf, eye, orbit.target);
}

} // namespace rig
