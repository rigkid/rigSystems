#include "SShapeRendering.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "CCamera.h"
#include "CDrawStyle.h"
#include "CMesh.h"
#include "CSelection.h"
#include "CShape.h"
#include "CTransform.h"
#include "SHierarchy.h"
#include "rendering/IRenderer.h"

namespace rigkit {
namespace ecs {
namespace {

constexpr float kTwoPi = 6.28318530717958647692f;

Paint fillPaint(const CDrawStyle& style) {
	return Paint::fill({style.fillR, style.fillG, style.fillB, style.fillA});
}

Paint strokePaint(const CDrawStyle& style) {
	return Paint::stroke({style.strokeR, style.strokeG, style.strokeB, style.strokeA},
						 style.strokeWidth);
}

/** Map local shape/mesh offset through CTransform::world (from SHierarchy). */
glm::vec2 toWorld(const CTransform& transform, float localX, float localY) {
	const glm::vec2 t = transform.worldTranslation2D();
	const glm::vec2 s = transform.worldScale2D();
	return {t.x + localX * s.x, t.y + localY * s.y};
}

/** Ring of @p count points alternating between @p outer and @p inner radius. */
std::vector<glm::vec2> radialPoints(glm::vec2 center, float outer, float inner, int count) {
	std::vector<glm::vec2> points;
	if (count < 3) {
		return points;
	}
	points.reserve(static_cast<size_t>(count));
	for (int i = 0; i < count; ++i) {
		const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(count);
		const float radius = (i % 2 == 0) ? outer : inner;
		points.emplace_back(center.x + radius * std::cos(angle),
							center.y + radius * std::sin(angle));
	}
	return points;
}

bool hasActiveCamera(MEcs& ecs) {
	auto cameras = ecs.view<ecs::CCamera>();
	for (auto entity : cameras) {
		if (cameras.get<ecs::CCamera>(entity).active) {
			return true;
		}
	}
	return false;
}

/**
 * Draw helpers bound to one renderer for a present pass. Fill and stroke are
 * separate draws with an explicit Paint, so call order never decides which one
 * a shape gets.
 */
struct Painter {
	IRenderer& out;

	void outline(const std::vector<glm::vec2>& points, const CDrawStyle& style) {
		if (style.hasFill) {
			out.drawPolygon(points, fillPaint(style));
		}
		if (style.hasStroke) {
			out.drawPolygon(points, strokePaint(style));
		}
	}

	void shape(const CTransform& transform, const CShape& shape, const CDrawStyle& style) {
		const glm::vec2 scale = transform.worldScale2D();
		const glm::vec2 origin = toWorld(transform, shape.x1, shape.y1);
		const float width = (shape.x2 - shape.x1) * scale.x;
		const float height = (shape.y2 - shape.y1) * scale.y;

		switch (shape.type) {
		case CShape::Type::Rectangle:
			if (style.hasFill) {
				out.drawRect(origin.x, origin.y, width, height, fillPaint(style));
			}
			if (style.hasStroke) {
				out.drawRect(origin.x, origin.y, width, height, strokePaint(style));
			}
			break;

		case CShape::Type::Ellipse: {
			const float centerX = origin.x + width * 0.5f;
			const float centerY = origin.y + height * 0.5f;
			if (style.hasFill) {
				out.drawEllipse(centerX, centerY, width, height, fillPaint(style));
			}
			if (style.hasStroke) {
				out.drawEllipse(centerX, centerY, width, height, strokePaint(style));
			}
			break;
		}

		case CShape::Type::Line:
			if (style.hasStroke) {
				const glm::vec2 end = toWorld(transform, shape.x2, shape.y2);
				out.drawLine(origin.x, origin.y, end.x, end.y, strokePaint(style));
			}
			break;

		case CShape::Type::Polygon: {
			const float radius = (shape.x2 - shape.x1) * scale.x;
			outline(radialPoints(origin, radius, radius, shape.sides), style);
			break;
		}

		case CShape::Type::Star: {
			const float outer = (shape.x2 - shape.x1) * scale.x;
			const float inner = outer * shape.innerRadius;
			outline(radialPoints(origin, outer, inner, shape.sides * 2), style);
			break;
		}
		}
	}

	void mesh(const CTransform& transform, const CMesh& mesh, const CDrawStyle& style) {
		if (mesh.positions.empty()) {
			return;
		}

		// The index buffer is optional — without one, vertices draw in order.
		const size_t count = mesh.indices.empty() ? mesh.positions.size() : mesh.indices.size();
		auto at = [&](size_t i) -> glm::vec2 {
			const uint32_t index =
				mesh.indices.empty() ? static_cast<uint32_t>(i) : mesh.indices[i];
			const glm::vec3& p = mesh.positions[index];
			return toWorld(transform, p.x, p.y);
		};
		auto segment = [&](size_t i0, size_t i1, const Paint& paint) {
			const glm::vec2 a = at(i0);
			const glm::vec2 b = at(i1);
			out.drawLine(a.x, a.y, b.x, b.y, paint);
		};

		if (mesh.mode == CMesh::Mode::Triangles) {
			if (style.hasFill) {
				const Paint paint = fillPaint(style);
				for (size_t i = 0; i + 2 < count; i += 3) {
					const glm::vec2 a = at(i);
					const glm::vec2 b = at(i + 1);
					const glm::vec2 c = at(i + 2);
					out.drawTriangle(a.x, a.y, b.x, b.y, c.x, c.y, paint);
				}
			}
			if (style.hasStroke) {
				const Paint paint = strokePaint(style);
				for (size_t i = 0; i + 2 < count; i += 3) {
					segment(i, i + 1, paint);
					segment(i + 1, i + 2, paint);
					segment(i + 2, i, paint);
				}
			}
			return;
		}

		if (!style.hasStroke && !style.hasFill) {
			return;
		}
		if (mesh.mode == CMesh::Mode::Lines || mesh.mode == CMesh::Mode::LineStrip) {
			const Paint paint = strokePaint(style);
			const size_t stride = (mesh.mode == CMesh::Mode::Lines) ? 2u : 1u;
			for (size_t i = 0; i + 1 < count; i += stride) {
				segment(i, i + 1, paint);
			}
		}
	}

	void bounds(float x, float y, float w, float h) {
		if (w <= 0.f || h <= 0.f) {
			return;
		}
		out.drawRect(x, y, w, h, Paint::stroke(glm::vec4(1.f, 0.85f, 0.2f, 1.f), 2.f));
	}
};

void selectionOverlay(MEcs& ecs, Painter& painter) {
	auto shapes = ecs.view<ecs::CTransform, ecs::CShape, ecs::CSelection>();
	for (auto entity : shapes) {
		const auto& selection = shapes.get<ecs::CSelection>(entity);
		if (!selection.isSelected && !selection.isMultiSelected) {
			continue;
		}
		const auto& transform = shapes.get<ecs::CTransform>(entity);
		const auto& shape = shapes.get<ecs::CShape>(entity);
		const glm::vec2 scale = transform.worldScale2D();
		const glm::vec2 origin = toWorld(transform, shape.x1, shape.y1);

		if (shape.type == ecs::CShape::Type::Line) {
			const float pad = 4.f;
			const glm::vec2 end = toWorld(transform, shape.x2, shape.y2);
			const float minX = std::min(origin.x, end.x);
			const float minY = std::min(origin.y, end.y);
			const float maxX = std::max(origin.x, end.x);
			const float maxY = std::max(origin.y, end.y);
			painter.bounds(minX - pad, minY - pad, (maxX - minX) + pad * 2.f,
						   (maxY - minY) + pad * 2.f);
		} else {
			painter.bounds(origin.x, origin.y, (shape.x2 - shape.x1) * scale.x,
						   (shape.y2 - shape.y1) * scale.y);
		}
	}

	if (hasActiveCamera(ecs)) {
		return;
	}

	auto meshes = ecs.view<ecs::CTransform, ecs::CMesh, ecs::CSelection>();
	for (auto entity : meshes) {
		const auto& selection = meshes.get<ecs::CSelection>(entity);
		if (!selection.isSelected && !selection.isMultiSelected) {
			continue;
		}
		const auto& transform = meshes.get<ecs::CTransform>(entity);
		const auto& mesh = meshes.get<ecs::CMesh>(entity);
		if (mesh.positions.empty()) {
			continue;
		}
		glm::vec2 min{1e9f, 1e9f};
		glm::vec2 max{-1e9f, -1e9f};
		for (const auto& p : mesh.positions) {
			const glm::vec2 world = toWorld(transform, p.x, p.y);
			min = glm::min(min, world);
			max = glm::max(max, world);
		}
		painter.bounds(min.x, min.y, max.x - min.x, max.y - min.y);
	}
}

} // namespace

void SShapeRendering(MEcs& ecs) {
	IRenderer* renderer = ecs.getPresentRenderer();
	if (!renderer) {
		return;
	}
	Painter painter{*renderer};

	// Refresh worlds here too — Canvas FBO present calls this during Draw
	// without re-entering Update.
	SHierarchy(ecs);

	auto shapes = ecs.view<ecs::CTransform, ecs::CShape, ecs::CDrawStyle>();
	for (auto entity : shapes) {
		painter.shape(shapes.get<ecs::CTransform>(entity), shapes.get<ecs::CShape>(entity),
					  shapes.get<ecs::CDrawStyle>(entity));
	}

	// Active camera ⇒ rigRender3D owns mesh present (keep 2D shape path for shapes).
	// Plot Toolpath 3D / scene-prop meshes stay in ECS for panel FBO present only —
	// never stroke them through the 2D IRenderer (that was crushing idle FPS).
	if (!hasActiveCamera(ecs)) {
		auto meshes = ecs.view<ecs::CTransform, ecs::CMesh, ecs::CDrawStyle>();
		for (auto entity : meshes) {
			const std::string name = ecs.entityName(entity);
			if (name.rfind("toolpath3d-", 0) == 0 || name.rfind("scene-prop:", 0) == 0) {
				continue;
			}
			painter.mesh(meshes.get<ecs::CTransform>(entity), meshes.get<ecs::CMesh>(entity),
						 meshes.get<ecs::CDrawStyle>(entity));
		}
	}

	selectionOverlay(ecs, painter);
}

} // namespace ecs
} // namespace rigkit
