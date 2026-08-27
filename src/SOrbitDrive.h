#pragma once

/**
 * @file
 * @brief Update system that poses a camera `CTransform` from its `COrbitDrive` fields.
 * @details This is the automatic turntable; mouse-driven orbit is `rig::orbitNavigate`.
 */

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Pose `CTransform` from `COrbitDrive` each Update (yaw around target).
 * @details Spherical eye via `pitch` / `yaw` / `radius`. Pause with `enabled`
 * false or `speed` 0. Mouse orbit / pan / dolly is `rig::orbitNavigate`.
 */
void SOrbitDrive(MEcs& ecs, float dt);

} // namespace ecs
} // namespace rigkit
