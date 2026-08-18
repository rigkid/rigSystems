#pragma once

#include <glm/glm.hpp>

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {
struct COrbitDrive;
struct CTransform;
} // namespace ecs
} // namespace rigkit

namespace rig {

constexpr float kOrbitPitchLimit = 1.4f;
constexpr float kOrbitRadiusMin = 0.5f;
constexpr float kOrbitRadiusMax = 40.f;

/**
 * @brief One Update of cooked orbit input. Bindings stay in the app.
 * @details Read GLFW / ImGui in the app, then fill this. `orbit` tumbles around
 * `COrbitDrive::target`. `pan` is truck + pedestal (slide target on the view
 * plane). `dolly` is drag-zoom (radius). `wheel` is a separate dolly. `blocked`
 * clears an in-progress drag (gizmo, UI, off-bed).
 */
struct OrbitNavFrame {
	float mouseX = 0.f;
	float mouseY = 0.f;
	float wheel = 0.f;
	bool blocked = false;
	bool orbit = false;
	bool pan = false;
	bool dolly = false;
};

/** @brief Drag last-xy. App-owned; not camera meaning. */
struct OrbitNavState {
	bool dragging = false;
	float lastX = 0.f;
	float lastY = 0.f;
};

/**
 * @brief Apply orbit / pan / dolly to `cam`'s `COrbitDrive`.
 * @details Turns auto-orbit off while dragging or wheeling. No-op without
 * `COrbitDrive` + `CTransform`. Does not read GLFW.
 */
void orbitNavigate(rigkit::MEcs& ecs, entt::entity cam, const OrbitNavFrame& frame,
				   OrbitNavState& state);

/** @brief Pose `CTransform` from `COrbitDrive` (clamps pitch / radius). */
void orbitApply(rigkit::MEcs& ecs, entt::entity cam);

/**
 * @brief Fit the orbit to a world AABB.
 * @return false when the box is empty.
 */
bool orbitFrame(rigkit::MEcs& ecs, entt::entity cam, const glm::vec3& bmin, const glm::vec3& bmax);

/**
 * @brief Frame selected triangle meshes, or all of them if none are selected.
 * @details Skips line meshes (grids). Calls `SHierarchy` first.
 */
bool orbitFrameMeshes(rigkit::MEcs& ecs, entt::entity cam);

/** @brief Write a view matrix back onto orbit + camera transform (view cube). */
void orbitFromView(rigkit::ecs::COrbitDrive& orbit, const glm::mat4& view,
				   rigkit::ecs::CTransform& camXf);

} // namespace rig
