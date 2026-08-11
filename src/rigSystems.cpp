#include "rigSystems.h"
#include <spdlog/spdlog.h>
#include "SCanvas.h"
#include "SHierarchy.h"
#include "SModulators.h"
#include "SOrbitDrive.h"
#include "SShapeRendering.h"
#include "STweens.h"
#include "core/RigKitEngine.h"
#include "core/canvas/MCanvas.h"
#include "core/pack/PackRegistry.h"
#include "ecs/MEcs.h"
#include "ecs/SystemRegistry.h"

namespace rigkit {

rigSystems::rigSystems() : IPack("rigSystems") {}

bool rigSystems::init() {
	spdlog::info("[rigSystems] init");
	return true;
}

void rigSystems::setup() {
	auto* engine = getEngine();
	if (!engine) {
		return;
	}
	auto* ecs = engine->getECSManager();
	if (!ecs) {
		return;
	}

	ecs->registerSystem("SHierarchy", SystemPhase::Update, rigkit::ecs::SHierarchy);
	ecs->registerSystem("SCanvasUpdate", SystemPhase::Update, rigkit::ecs::SCanvasUpdate);
	ecs->registerSystem("SModulators", SystemPhase::Update, rigkit::ecs::SModulators);
	ecs->registerSystem("STweens", SystemPhase::Update, rigkit::ecs::STweens);
	ecs->registerSystem("SOrbitDrive", SystemPhase::Update, rigkit::ecs::SOrbitDrive);

	// FBO / active Canvas present (no-op when no active canvas).
	ecs->registerSystem("SCanvasRender", SystemPhase::Draw, [engine](MEcs& e) {
		rigkit::ecs::SCanvasRender(e, engine->getCanvasManager());
	});

	ecs->registerSystem("SShapeRendering", SystemPhase::Draw, rigkit::ecs::SShapeRendering);

	spdlog::info("[rigSystems] registered Update/Draw systems");
}

} // namespace rigkit

namespace {
struct rigSystemsRegistrar {
	rigSystemsRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigSystems", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigSystems>());
		});
	}
};
static rigSystemsRegistrar rigSystems_auto_reg;
} // namespace
