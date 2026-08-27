#pragma once

/**
 * @file
 * @brief Canvas systems: advance the `CCanvas` clock, then present shapes into its FBO.
 * @details The Update half only moves numbers; only `SCanvasRender` binds the host Canvas.
 */

#include "ecs/MEcs.h"

namespace rigkit {
class MCanvas;
namespace ecs {

/**
 * @brief Advance CCanvas clock fields (`time` / `frameCount`).
 * @details Does not touch the host Canvas / FBO - that path is Draw-only
 * via SCanvasRender.
 */
void SCanvasUpdate(MEcs& ecs, float deltaTime);

/**
 * @brief Present ECS shapes into the active Canvas FBO (if any).
 * @details Safe during Draw: calls SShapeRendering directly - does not
 * re-enter renderSystems().
 */
void SCanvasRender(MEcs& ecs, MCanvas* canvasManager);

} // namespace ecs
} // namespace rigkit
