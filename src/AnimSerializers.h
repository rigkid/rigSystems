#pragma once

/**
 * @file
 * @brief Register tween / LFO / binding codecs on a document serializer registry.
 */

namespace rigkit {
namespace project {
class ComponentSerializerRegistry;
}

namespace anim {

void registerProjectSerializers(project::ComponentSerializerRegistry& registry);

} // namespace anim
} // namespace rigkit
