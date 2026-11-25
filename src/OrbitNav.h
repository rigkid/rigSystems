#pragma once

/**
 * @file
 * @brief Orbit camera navigation: tumble, pan, dolly, pivot picking, and framing.
 * @details Nothing here reads input devices - the app fills an `OrbitNavFrame` each frame and
 * owns the `OrbitNavState` that carries a drag between frames. This is the only
 * mouse-to-orbit owner: do not invent panel-local truck/tumble signs. Present Y
 * comes from `OrbitNavFrame::presentOrigin` (`TextureYOrigin`).
 */

#include <glm/glm.hpp>

#include "ecs/MEcs.h"
#include "rendering/TextureYOrigin.h"

namespace rigkit {
namespace ecs {
struct COrbitDrive;
struct CTransform;
} // namespace ecs
} // namespace rigkit

namespace rig {

/// Soft stop short of ±pi/2 so lookAt stays stable; still allows Host Space Top
/// (`pi/2 - 0.02` ≈ 1.5508) through `orbitApply` / tumble.
constexpr float kOrbitPitchLimit = 1.551f;

/**
 * @brief One Update of cooked orbit input. Bindings stay in the app.
 * @details Read GLFW / ImGui in the app, then fill this. `orbit` tumbles around
 * `COrbitDrive::target`. `pan` is truck + pedestal (slide target on the view
 * plane). With `viewH` set, pan tracks the screen so DPI and zoom stay natural;
 * otherwise it uses a radius heuristic. `dolly` / `wheel` zoom: perspective
 * dollies `radius`; orthographic scales `CCamera::orthoHeight` and leaves
 * `radius` as the clip standoff. `wheel` applies when `blocked`. `blocked`
 * clears an in-progress drag (gizmo, UI, off-bed). With `havePivot`, orbit
 * tumbles about `pivot` (captured at press) and wheel zoom anchors on it, so
 * the content under the cursor keeps its screen position.
 *
 * `presentOrigin` selects truck vertical sign for the blit path: BottomLeft
 * (default, FBO V-flip or window GL) matches grab-pan used by Toolpath 3D and
 * RigSole; TopLeft is for identity-UV TopLeft attachments.
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
	rigkit::TextureYOrigin presentOrigin = rigkit::TextureYOrigin::BottomLeft;
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
 * @brief Grab-truck target delta for one mouse step (shared with OrthoView pan).
 * @details BottomLeft: `(-dx*right + dy*up) * worldPerPx`. TopLeft flips the
 * vertical term. Same table as Toolpath 3D after the V-flip settle.
 */
inline glm::vec3 orbitTruckDelta(rigkit::TextureYOrigin origin, const glm::vec3& right,
								 const glm::vec3& up, float mouseDx, float mouseDy,
								 float worldPerPx) {
	const float ySign = rigkit::presentMouseYMatchesCameraUp(origin) ? -1.f : 1.f;
	return (-mouseDx * right + ySign * mouseDy * up) * worldPerPx;
}

/**
 * @brief Apply orbit / pan / dolly to `cam`'s `COrbitDrive`.
 * @details Turns auto-orbit off while dragging or wheeling. Ortho zoom is
 * `orthoHeight`, not a walk into the scene. No-op without `COrbitDrive` +
 * `CTransform`. Does not read GLFW. Yaw / pitch follow `COrbitDrive::worldUp`.
 */
void orbitNavigate(rigkit::MEcs& ecs, entt::entity cam, const OrbitNavFrame& frame,
				   OrbitNavState& state);

/**
 * @brief Pose `CTransform` from `COrbitDrive` (clamps pitch / radius).
 * @details Writes near / far from the standoff so ortho zoom cannot hide the
 * scene behind the clip planes. `lookAt` uses `worldUp`.
 */
void orbitApply(rigkit::MEcs& ecs, entt::entity cam);

/**
 * @brief World pivot under a viewport point, for `OrbitNavFrame::pivot`.
 * @details Casts the pixel ray from `cam` (`ndcX` / `ndcY` in -1..1, +Y up;
 * works for perspective and ortho). Hit the ground plane through `target`
 * (normal = `worldUp`) when the ray lands in front at sane range; otherwise
 * the view plane through `COrbitDrive::target`. Convert widget Y with
 * `presentNdcYFromWidgetLocal` — do not invent a third NDC map.
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

/**
 * @brief Spherical eye from yaw/pitch/radius about `target` for `worldUp`.
 * @details Y-up: `(sy*cp, sp, cy*cp)*r` (CAD). Z-up Host Space: operator at
 * low document Y — `(sy*cp, -cy*cp, sp)*r` so yaw 0 / pitch 0 is Front.
 */
glm::vec3 orbitEye(const rigkit::ecs::COrbitDrive& orbit);

/** @brief Camera right / up basis for the current yaw/pitch and worldUp. */
void orbitViewAxes(const rigkit::ecs::COrbitDrive& orbit, glm::vec3& outRight, glm::vec3& outUp);

} // namespace rig
