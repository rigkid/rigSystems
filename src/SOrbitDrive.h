#pragma once

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Pose `CTransform` from `COrbitDrive` each Update (yaw around target).
 * @details Writes eye position + lookAt rotation. Pause by setting `enabled` false
 * or `speed` to 0. Hosts should not hand-roll the same lookAt loop.
 */
void SOrbitDrive(MEcs& ecs, float dt);

} // namespace ecs
} // namespace rigkit
