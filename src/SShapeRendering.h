#pragma once

/**
 * @file
 * @brief Draw pass for shapes, meshes, and selection bounds via the present `IRenderer`.
 * @details The host sets that renderer before calling in; meshes with an active `CCamera`
 * are left to `rigRender3D`.
 */

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Draw shapes, meshes, then selection bounds via the present IRenderer.
 * @details Takes the renderer from `MEcs::getPresentRenderer()`, which the host
 * Draw pass (or a Canvas FBO pass) sets first. No-op without one. 2D meshes
 * go through `drawTriangles` / `drawLines` so the host can batch them. Skips
 * meshes when an active `CCamera` exists — `rigRender3D` presents them.
 */
void SShapeRendering(MEcs& ecs);

} // namespace ecs
} // namespace rigkit
