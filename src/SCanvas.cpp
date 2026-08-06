#include "SCanvas.h"
#include "CCanvas.h"
#include "SShapeRendering.h"
#include "core/canvas/Canvas.h"
#include "core/canvas/MCanvas.h"
#include "rendering/Graphics.h"
#include "rendering/IRenderer.h"

namespace rigkit {
namespace ecs {

void SCanvasUpdate(MEcs& ecs, float deltaTime) {
	for (auto entity : ecs.view<CCanvas>()) {
		auto& canvas = ecs.getComponent<CCanvas>(entity);
		canvas.time += deltaTime;
		++canvas.frameCount;
	}
}

void SCanvasRender(MEcs& ecs, MCanvas* canvasManager) {
	if (!canvasManager) {
		return;
	}
	auto canvas = canvasManager->getActiveCanvas();
	if (!canvas) {
		return;
	}

	if (!canvas->getGraphics()) {
		return;
	}
	if (!canvas->getGraphics()->getRenderer()) {
		canvas->switchRenderer(RendererType::OpenGL);
	}
	IRenderer* renderer = canvas->getGraphics()->getRenderer();
	if (!renderer) {
		return;
	}

	IRenderer* previous = ecs.getPresentRenderer();
	ecs.setPresentRenderer(renderer);
	canvas->beginOffscreen();
	renderer->beginFrame();
	canvas->getGraphics()->clear(1.f, 1.f, 1.f, 1.f);
	SShapeRendering(ecs);
	renderer->endFrame();
	canvas->endOffscreen();
	ecs.setPresentRenderer(previous);
}

} // namespace ecs
} // namespace rigkit
