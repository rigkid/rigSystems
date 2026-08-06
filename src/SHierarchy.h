#pragma once

#include "ecs/MEcs.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Resolve CTransform::world from local TRS + optional CRelationship.parent.
 * @details Run in Update before Draw. Missing CRelationship ⇒ root (world = local).
 * Visit buffers reuse capacity across frames; every CTransform is still walked.
 */
void SHierarchy(MEcs& ecs);

} // namespace ecs
} // namespace rigkit
