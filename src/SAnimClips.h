#pragma once

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Advance `CAnimClip` clocks and sample `CAnimTrack` onto targets.
 * @details Supports `propertyKey` `rotation` (quat stride 4) and `position`
 * (vec3 stride 3) on `CTransform`. Wrap modes: once, loop, ping-pong.
 */
void SAnimClips(MEcs& ecs, float dt);

} // namespace ecs
} // namespace rigkit
