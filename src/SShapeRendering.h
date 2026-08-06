#pragma once

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Draw shapes, meshes, then selection bounds via the present IRenderer.
 * @details Takes the renderer from `MEcs::getPresentRenderer()`, which the host
 * Draw pass (or a Canvas FBO pass) sets first. No-op without one. Skips meshes
 * when an active `CCamera` exists — `rigRender3D` presents them.
 */
void SShapeRendering(MEcs& ecs);

} // namespace ecs
} // namespace rigkit
