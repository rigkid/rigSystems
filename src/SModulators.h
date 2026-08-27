#pragma once

/**
 * @file
 * @brief Update system that drives bound properties from `CModLfo` sources.
 * @details Writes through `GetProperties()`, so any registered field can be a target.
 */

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Sample `CModLfo` sources and apply `CModBinding` writes each Update.
 * @details Fulfillment for `rig.mod.lfo` + `rig.mod.binding`. Uses
 * `writeEntityProperty` (GetProperties). Hosts that run Update systems need no
 * free-function tick.
 */
void SModulators(MEcs& ecs, float dt);

} // namespace ecs
} // namespace rigkit
