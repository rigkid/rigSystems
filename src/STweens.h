#pragma once

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Advance `CTween` and write `target`/`propertyKey` each Update.
 * @details Fulfillment for `rig.anim.tween` via `writeEntityProperty`.
 */
void STweens(MEcs& ecs, float dt);

} // namespace ecs
} // namespace rigkit
