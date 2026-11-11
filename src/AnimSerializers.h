#pragma once

/**
 * @file
 * @brief Register anim / mod codecs on a document serializer registry.
 * @details Covers tween / LFO / binding plus skeleton / bind / skin / clip / track.
 */

namespace rigkit {
namespace project {
class ComponentSerializerRegistry;
}

namespace anim {

void registerProjectSerializers(project::ComponentSerializerRegistry& registry);

} // namespace anim
} // namespace rigkit
