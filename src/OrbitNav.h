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

/**
 * @brief One Update of cooked orbit input. Bindings stay in the app.
 * @details Read GLFW / ImGui in the app, then fill this. `orbit` tumbles around
 * `COrbitDrive::target`. `pan` is truck + pedestal (slide target on the view
 * plane). With `viewH` set, pan tracks the screen so DPI and zoom stay natural;
 * otherwise it uses a radius heuristic. `dolly` is drag-zoom (radius). `wheel`
 * applies when `blocked`. `blocked` clears an in-progress drag (gizmo, UI, off-bed).
 * With `havePivot`, orbit tumbles about `pivot` (captured at press) and wheel
 * zoom anchors on it, so the content under the cursor keeps its screen position.
 */
struct OrbitNavFrame {
	float mouseX = 0.f;
	float mouseY = 0.f;
	float wheel = 0.f;
	bool blocked = false;
	bool orbit = false;
	bool pan = false;
	bool dolly = false;
	/// World point under the cursor (see `orbitPickPivot`).
	glm::vec3 pivot{0.f};
	bool havePivot = false;
	/// Viewport size in the same units as `mouseX` / `mouseY` (GLFW pixels).
	/// When `viewH` > 1, pan matches on-screen motion at the orbit target
	/// (FOV / ortho height); otherwise falls back to a radius heuristic.
	float viewW = 0.f;
	float viewH = 0.f;
};

/** @brief Drag last-xy + pivot captured at press. App-owned; not camera meaning. */
struct OrbitNavState {
	bool dragging = false;
	float lastX = 0.f;
	float lastY = 0.f;
	glm::vec3 pivot{0.f};
	bool havePivot = false;
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
 * @brief World pivot under a viewport point, for `OrbitNavFrame::pivot`.
 * @details Casts the pixel ray from `cam` (`ndcX` / `ndcY` in -1..1, +Y up;
 * works for perspective and ortho). Returns the ground-plane hit (Y = 0) when
 * the ray lands there in front of the camera at sane range — a grazing ray
 * would put the pivot near the horizon — otherwise falls back to the view
 * plane through `COrbitDrive::target`.
 */
glm::vec3 orbitPickPivot(rigkit::MEcs& ecs, entt::entity cam, float ndcX, float ndcY, float aspect);

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
