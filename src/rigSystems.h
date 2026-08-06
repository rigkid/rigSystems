#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief ECS systems pack — Update/Draw fulfillment over rigComponent data.
 * @details Depends on rigComponent. Registers SCanvasUpdate, SCanvasRender,
 * SShapeRendering (including selection overlay).
 */
class rigSystems : public IPack {
  public:
	rigSystems();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
