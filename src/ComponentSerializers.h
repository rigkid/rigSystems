#pragma once

/**
 * @file
 * @brief Floor sketch codecs for `rigComponent` PODs - filed by **rigSystems**.
 */

namespace rigkit {
namespace project {

class ComponentSerializerRegistry;

/** @brief Register rigComponent's portable POD codecs on a document registry. */
void registerComponentSerializers(ComponentSerializerRegistry& registry);

} // namespace project
} // namespace rigkit
