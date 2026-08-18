#include "SShapeRendering.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "CArc.h"
#include "CCamera.h"
#include "CDrawStyle.h"
#include "CLayer.h"
#include "CRelationship.h"
#include "CEllipse.h"
#include "CLine.h"
#include "CMesh.h"
#include "CPolygon.h"
#include "CRectangle.h"
#include "CRegularPolygon.h"
#include "CRing.h"
#include "CSelection.h"
#include "CSpline.h"
#include "CStar.h"
#include "CTransform.h"
#include "PrimitiveBounds.h"
#include "PrimitiveVertices.h"
#include "SHierarchy.h"
#include "rendering/IRenderer.h"

namespace rigkit {
namespace ecs {
namespace {

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

/** Map a whole local point list through CTransform::world. */
std::vector<glm::vec2> toWorldAll(const CTransform& transform,
								  const std::vector<glm::vec2>& local) {
	std::vector<glm::vec2> world;
	world.reserve(local.size());
	for (const auto& p : local) {
		world.push_back(toWorld(transform, p.x, p.y));
	}
	return world;
}

bool hiddenByLayer(MEcs& ecs, entt::entity e) {
	auto& reg = ecs.registry();
	int guard = 0;
	while (e != entt::null && reg.valid(e) && guard++ < 64) {
		if (reg.all_of<ecs::CLayer>(e) && !reg.get<ecs::CLayer>(e).visible) {
			return true;
		}
		if (!reg.all_of<ecs::CRelationship>(e)) {
			break;
		}
		e = reg.get<ecs::CRelationship>(e).parent;
	}
	return false;
}

/** Draw every entity carrying @p TShape through @p draw. */
template <typename TShape, typename TDraw> void presentShapes(MEcs& ecs, TDraw&& draw) {
	auto view = ecs.view<ecs::CTransform, TShape, ecs::CDrawStyle>();
	for (auto entity : view) {
		if (hiddenByLayer(ecs, entity)) {
			continue;
		}
		draw(view.template get<ecs::CTransform>(entity), view.template get<TShape>(entity),
			 view.template get<ecs::CDrawStyle>(entity));
	}
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

	/** Open run of segments — no closing edge back to the first point. */
	void polyline(const std::vector<glm::vec2>& points, const CDrawStyle& style) {
		if (!style.hasStroke || points.size() < 2) {
			return;
		}
		const Paint paint = strokePaint(style);
		for (size_t i = 0; i + 1 < points.size(); ++i) {
			out.drawLine(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y, paint);
		}
	}

	void rectangle(const CTransform& transform, const CRectangle& shape, const CDrawStyle& style) {
		const glm::vec2 scale = transform.worldScale2D();
		const glm::vec2 origin = toWorld(transform, shape.x, shape.y);
		const float width = shape.width * scale.x;
		const float height = shape.height * scale.y;
		// IRenderer has no rounded-rect primitive, so cornerRadius survives the
		// round trip but does not yet show in the present pass.
		if (style.hasFill) {
			out.drawRect(origin.x, origin.y, width, height, fillPaint(style));
		}
		if (style.hasStroke) {
			out.drawRect(origin.x, origin.y, width, height, strokePaint(style));
		}
	}

	void ellipse(const CTransform& transform, const CEllipse& shape, const CDrawStyle& style) {
		const glm::vec2 scale = transform.worldScale2D();
		const glm::vec2 center = toWorld(transform, shape.cx, shape.cy);
		const float width = shape.rx * 2.0f * scale.x;
		const float height = shape.ry * 2.0f * scale.y;
		if (style.hasFill) {
			out.drawEllipse(center.x, center.y, width, height, fillPaint(style));
		}
		if (style.hasStroke) {
			out.drawEllipse(center.x, center.y, width, height, strokePaint(style));
		}
	}

	void line(const CTransform& transform, const CLine& shape, const CDrawStyle& style) {
		if (!style.hasStroke) {
			return;
		}
		const glm::vec2 start = toWorld(transform, shape.x1, shape.y1);
		const glm::vec2 end = toWorld(transform, shape.x2, shape.y2);
		out.drawLine(start.x, start.y, end.x, end.y, strokePaint(style));
	}

	void polygon(const CTransform& transform, const CPolygon& shape, const CDrawStyle& style) {
		if (shape.points.size() < 3) {
			return;
		}
		const auto world = toWorldAll(transform, shape.points);
		if (shape.closed) {
			outline(world, style);
		} else {
			polyline(world, style);
		}
	}

	void regularPolygon(const CTransform& transform, const CRegularPolygon& shape,
						const CDrawStyle& style) {
		outline(toWorldAll(transform, verticesOf(shape)), style);
	}

	void star(const CTransform& transform, const CStar& shape, const CDrawStyle& style) {
		outline(toWorldAll(transform, verticesOf(shape)), style);
	}

	void arc(const CTransform& transform, const CArc& shape, const CDrawStyle& style) {
		const auto world = toWorldAll(transform, verticesOf(shape));
		// A pie already ends at the centre, so closing it completes the wedge.
		if (shape.pie) {
			outline(world, style);
		} else {
			polyline(world, style);
		}
	}

	void spline(const CTransform& transform, const CSpline& shape, const CDrawStyle& style) {
		polyline(toWorldAll(transform, verticesOf(shape)), style);
	}

	void ring(const CTransform& transform, const CRing& shape, const CDrawStyle& style) {
		std::vector<glm::vec2> outerLocal;
		std::vector<glm::vec2> innerLocal;
		rimsOf(shape, outerLocal, innerLocal);
		if (outerLocal.empty()) {
			return;
		}
		const auto outerWorld = toWorldAll(transform, outerLocal);
		const auto innerWorld = toWorldAll(transform, innerLocal);

		if (style.hasFill) {
			// No annulus primitive exists, so the hole comes from banding the gap
			// between the rims rather than overdrawing a disc.
			const Paint paint = fillPaint(style);
			const size_t count = outerWorld.size();
			for (size_t i = 0; i < count; ++i) {
				const size_t next = (i + 1) % count;
				out.drawTriangle(outerWorld[i].x, outerWorld[i].y, outerWorld[next].x,
								 outerWorld[next].y, innerWorld[i].x, innerWorld[i].y, paint);
				out.drawTriangle(innerWorld[i].x, innerWorld[i].y, outerWorld[next].x,
								 outerWorld[next].y, innerWorld[next].x, innerWorld[next].y,
								 paint);
			}
		}
		if (style.hasStroke) {
			const Paint paint = strokePaint(style);
			out.drawPolygon(outerWorld, paint);
			out.drawPolygon(innerWorld, paint);
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
	auto shapes = ecs.view<ecs::CTransform, ecs::CSelection>();
	for (auto entity : shapes) {
		const auto& selection = shapes.get<ecs::CSelection>(entity);
		if (!selection.isSelected && !selection.isMultiSelected) {
			continue;
		}
		if (hiddenByLayer(ecs, entity)) {
			continue;
		}
		const Bounds2D local = shapeBounds2D(ecs, entity);
		if (!local.valid) {
			continue;
		}
		const auto& transform = shapes.get<ecs::CTransform>(entity);
		const glm::vec2 min = toWorld(transform, local.min.x, local.min.y);
		const glm::vec2 max = toWorld(transform, local.max.x, local.max.y);
		// A line or a flat arc has no area on one axis, so an unpadded box would
		// collapse and never draw.
		const float pad = ((max.x - min.x) < 1.f || (max.y - min.y) < 1.f) ? 4.f : 0.f;
		painter.bounds(min.x - pad, min.y - pad, (max.x - min.x) + pad * 2.f,
					   (max.y - min.y) + pad * 2.f);
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
		if (hiddenByLayer(ecs, entity)) {
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

	using Xf = ecs::CTransform;
	using Style = ecs::CDrawStyle;
	presentShapes<ecs::CRectangle>(
		ecs, [&](const Xf& xf, const ecs::CRectangle& s, const Style& st) {
			painter.rectangle(xf, s, st);
		});
	presentShapes<ecs::CEllipse>(
		ecs, [&](const Xf& xf, const ecs::CEllipse& s, const Style& st) {
			painter.ellipse(xf, s, st);
		});
	presentShapes<ecs::CLine>(
		ecs, [&](const Xf& xf, const ecs::CLine& s, const Style& st) { painter.line(xf, s, st); });
	presentShapes<ecs::CPolygon>(
		ecs, [&](const Xf& xf, const ecs::CPolygon& s, const Style& st) {
			painter.polygon(xf, s, st);
		});
	presentShapes<ecs::CRegularPolygon>(
		ecs, [&](const Xf& xf, const ecs::CRegularPolygon& s, const Style& st) {
			painter.regularPolygon(xf, s, st);
		});
	presentShapes<ecs::CStar>(
		ecs, [&](const Xf& xf, const ecs::CStar& s, const Style& st) { painter.star(xf, s, st); });
	presentShapes<ecs::CArc>(
		ecs, [&](const Xf& xf, const ecs::CArc& s, const Style& st) { painter.arc(xf, s, st); });
	presentShapes<ecs::CSpline>(
		ecs, [&](const Xf& xf, const ecs::CSpline& s, const Style& st) {
			painter.spline(xf, s, st);
		});
	presentShapes<ecs::CRing>(
		ecs, [&](const Xf& xf, const ecs::CRing& s, const Style& st) { painter.ring(xf, s, st); });

	// Active camera ⇒ rigRender3D owns mesh present (keep 2D shape path for shapes).
	// Plot Toolpath 3D / scene-prop meshes stay in ECS for panel FBO present only —
	// never stroke them through the 2D IRenderer (that was crushing idle FPS).
	if (!hasActiveCamera(ecs)) {
		auto meshes = ecs.view<ecs::CTransform, ecs::CMesh, ecs::CDrawStyle>();
		for (auto entity : meshes) {
			if (hiddenByLayer(ecs, entity)) {
				continue;
			}
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
