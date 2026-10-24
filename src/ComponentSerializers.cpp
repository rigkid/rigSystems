#include "ComponentSerializers.h"

#include "core/TypeJson.h"
#include "ecs/components/CGuide.h"
#include "AddSerializer.h"
#include "CArc.h"
#include "CAssetRef.h"
#include "CCamera.h"
#include "CCurve.h"
#include "CDrawStyle.h"
#include "CEdgeSelection.h"
#include "CEllipse.h"
#include "CFaceSelection.h"
#include "CGroup.h"
#include "CIndexedAtlas.h"
#include "CLayer.h"
#include "CLight.h"
#include "CLine.h"
#include "CMesh.h"
#include "CNurbsSurface.h"
#include "CPalette.h"
#include "CPath.h"
#include "CPolygon.h"
#include "CRectangle.h"
#include "CRegularPolygon.h"
#include "CRelationship.h"
#include "CRenderBlend.h"
#include "CRenderEnvironment.h"
#include "CRenderVisibility.h"
#include "CRing.h"
#include "CSelectable.h"
#include "CSpline.h"
#include "CSpline3d.h"
#include "CStar.h"
#include "CText.h"
#include "CTextPath.h"
#include "CFontFace.h"
#include "CInstance.h"
#include "CPaintClip.h"
#include "CPaintFilter.h"
#include "CPaintFilterStep.h"
#include "CPaintMask.h"
#include "CPaintMarker.h"
#include "CPaintMarkers.h"
#include "CPaintOrder.h"
#include "CPaintPattern.h"
#include "CTransform.h"
#include "CUiPanel.h"
#include "EntityIdRemap.h"
#include "MeshEdge.h"
#include "MeshFaces.h"
#include "PathEllipticArc.h"
#include "ProjectJson.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace rigkit {
namespace {

using project::entityIdFromString;
using project::EntityIdMap;
using project::entityIdString;
using project::ordered_json;
bool serializeTransform(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CTransform>(e)) {
		return false;
	}
	const auto& t = reg.get<ecs::CTransform>(e);
	j["position"] = vec3ToJson(t.position);
	j["rotation"] = quatToJson(t.rotation);
	j["scale"] = vec3ToJson(t.scale);
	if (t.shear != glm::vec3(0.0f)) {
		j["shear"] = vec3ToJson(t.shear); // Optional in the schema - absent = none.
	}
	// Omit editor euler and derived world matrix.
	return true;
}

bool deserializeTransform(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CTransform t;
	if (j.contains("position")) {
		t.position = vec3FromJson(j["position"], t.position);
	}
	if (j.contains("scale")) {
		t.scale = vec3FromJson(j["scale"], t.scale);
	}
	if (j.contains("shear")) {
		t.shear = vec3FromJson(j["shear"], t.shear);
	}
	if (j.contains("rotation")) {
		t.setRotationQuat(quatFromJson(j["rotation"], t.rotation));
	}
	t.world = glm::mat4(1.0f);
	reg.emplace_or_replace<ecs::CTransform>(e, t);
	return true;
}

// Field names below are the Contract's own; the writer keys each blob by schema id.

bool serializeRectangle(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CRectangle>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CRectangle>(e);
	j["x"] = s.x;
	j["y"] = s.y;
	j["width"] = s.width;
	j["height"] = s.height;
	j["cornerRadius"] = s.cornerRadius;
	return true;
}

bool deserializeRectangle(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CRectangle s;
	s.x = j.value("x", s.x);
	s.y = j.value("y", s.y);
	s.width = j.value("width", s.width);
	s.height = j.value("height", s.height);
	s.cornerRadius = j.value("cornerRadius", s.cornerRadius);
	reg.emplace_or_replace<ecs::CRectangle>(e, s);
	return true;
}

bool serializeEllipse(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CEllipse>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CEllipse>(e);
	j["cx"] = s.cx;
	j["cy"] = s.cy;
	j["rx"] = s.rx;
	j["ry"] = s.ry;
	return true;
}

bool deserializeEllipse(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CEllipse s;
	s.cx = j.value("cx", s.cx);
	s.cy = j.value("cy", s.cy);
	s.rx = j.value("rx", s.rx);
	s.ry = j.value("ry", s.ry);
	reg.emplace_or_replace<ecs::CEllipse>(e, s);
	return true;
}

bool serializeLine(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CLine>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CLine>(e);
	j["x1"] = s.x1;
	j["y1"] = s.y1;
	j["x2"] = s.x2;
	j["y2"] = s.y2;
	return true;
}

bool deserializeLine(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CLine s;
	s.x1 = j.value("x1", s.x1);
	s.y1 = j.value("y1", s.y1);
	s.x2 = j.value("x2", s.x2);
	s.y2 = j.value("y2", s.y2);
	reg.emplace_or_replace<ecs::CLine>(e, s);
	return true;
}

bool serializePolygon(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPolygon>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CPolygon>(e);
	auto points = ordered_json::array();
	for (const auto& p : s.points) {
		points.push_back(vec2ToJson(p));
	}
	j["points"] = std::move(points);
	j["closed"] = s.closed;
	return true;
}

bool deserializePolygon(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPolygon s;
	if (auto it = j.find("points"); it != j.end() && it->is_array()) {
		s.points.reserve(it->size());
		for (const auto& p : *it) {
			s.points.push_back(vec2FromJson(p));
		}
	}
	s.closed = j.value("closed", s.closed);
	reg.emplace_or_replace<ecs::CPolygon>(e, std::move(s));
	return true;
}

bool serializeRegularPolygon(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CRegularPolygon>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CRegularPolygon>(e);
	j["cx"] = s.cx;
	j["cy"] = s.cy;
	j["radius"] = s.radius;
	j["sides"] = s.sides;
	j["rotationDegrees"] = s.rotationDegrees;
	return true;
}

bool deserializeRegularPolygon(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CRegularPolygon s;
	s.cx = j.value("cx", s.cx);
	s.cy = j.value("cy", s.cy);
	s.radius = j.value("radius", s.radius);
	s.sides = j.value("sides", s.sides);
	s.rotationDegrees = j.value("rotationDegrees", s.rotationDegrees);
	reg.emplace_or_replace<ecs::CRegularPolygon>(e, s);
	return true;
}

bool serializeStar(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CStar>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CStar>(e);
	j["cx"] = s.cx;
	j["cy"] = s.cy;
	j["radius"] = s.radius;
	j["innerRadius"] = s.innerRadius;
	j["points"] = s.points;
	j["rotationDegrees"] = s.rotationDegrees;
	return true;
}

bool deserializeStar(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CStar s;
	s.cx = j.value("cx", s.cx);
	s.cy = j.value("cy", s.cy);
	s.radius = j.value("radius", s.radius);
	s.innerRadius = j.value("innerRadius", s.innerRadius);
	s.points = j.value("points", s.points);
	s.rotationDegrees = j.value("rotationDegrees", s.rotationDegrees);
	reg.emplace_or_replace<ecs::CStar>(e, s);
	return true;
}

bool serializeArc(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CArc>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CArc>(e);
	j["cx"] = s.cx;
	j["cy"] = s.cy;
	j["rx"] = s.rx;
	j["ry"] = s.ry;
	if (s.isCircular()) {
		j["radius"] = s.rx;
	}
	j["startAngleDegrees"] = s.startAngleDegrees;
	j["endAngleDegrees"] = s.endAngleDegrees;
	j["pie"] = s.pie;
	return true;
}

bool deserializeArc(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CArc s;
	s.cx = j.value("cx", s.cx);
	s.cy = j.value("cy", s.cy);
	if (j.contains("rx") || j.contains("ry")) {
		s.rx = j.value("rx", j.value("radius", s.rx));
		s.ry = j.value("ry", j.value("radius", s.ry));
	} else {
		s.setRadius(j.value("radius", s.rx));
	}
	s.startAngleDegrees = j.value("startAngleDegrees", s.startAngleDegrees);
	s.endAngleDegrees = j.value("endAngleDegrees", s.endAngleDegrees);
	s.pie = j.value("pie", s.pie);
	reg.emplace_or_replace<ecs::CArc>(e, s);
	return true;
}

bool serializeSpline(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CSpline>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CSpline>(e);
	j["degree"] = s.degree;
	j["closed"] = s.closed;
	j["controlPoints"] = ordered_json::array();
	for (const auto& p : s.controlPoints) {
		j["controlPoints"].push_back({p.x, p.y});
	}
	j["knots"] = s.knots;
	if (!s.weights.empty()) {
		j["weights"] = s.weights;
	}
	if (!s.fitPoints.empty()) {
		j["fitPoints"] = ordered_json::array();
		for (const auto& p : s.fitPoints) {
			j["fitPoints"].push_back({p.x, p.y});
		}
	}
	return true;
}

bool deserializeSpline(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CSpline s;
	s.degree = j.value("degree", s.degree);
	s.closed = j.value("closed", s.closed);
	if (j.contains("controlPoints") && j["controlPoints"].is_array()) {
		for (const auto& p : j["controlPoints"]) {
			s.controlPoints.push_back(vec2FromJson(p));
		}
	}
	if (j.contains("knots") && j["knots"].is_array()) {
		for (const auto& k : j["knots"]) {
			s.knots.push_back(k.get<float>());
		}
	}
	if (j.contains("weights") && j["weights"].is_array()) {
		for (const auto& w : j["weights"]) {
			s.weights.push_back(w.get<float>());
		}
	}
	if (j.contains("fitPoints") && j["fitPoints"].is_array()) {
		for (const auto& p : j["fitPoints"]) {
			s.fitPoints.push_back(vec2FromJson(p));
		}
	}
	reg.emplace_or_replace<ecs::CSpline>(e, std::move(s));
	return true;
}

bool serializeSpline3d(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CSpline3d>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CSpline3d>(e);
	j["degree"] = s.degree;
	if (s.closed) {
		j["closed"] = true;
	}
	j["controlPoints"] = ordered_json::array();
	for (const auto& p : s.controlPoints) {
		j["controlPoints"].push_back(vec3ToJson(p));
	}
	j["knots"] = s.knots;
	if (!s.weights.empty()) {
		j["weights"] = s.weights;
	}
	if (!s.fitPoints.empty()) {
		j["fitPoints"] = ordered_json::array();
		for (const auto& p : s.fitPoints) {
			j["fitPoints"].push_back(vec3ToJson(p));
		}
	}
	return true;
}

bool deserializeSpline3d(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CSpline3d s;
	s.degree = j.value("degree", s.degree);
	s.closed = j.value("closed", s.closed);
	if (j.contains("controlPoints") && j["controlPoints"].is_array()) {
		for (const auto& p : j["controlPoints"]) {
			s.controlPoints.push_back(vec3FromJson(p));
		}
	}
	if (j.contains("knots") && j["knots"].is_array()) {
		for (const auto& k : j["knots"]) {
			s.knots.push_back(k.get<float>());
		}
	}
	if (j.contains("weights") && j["weights"].is_array()) {
		for (const auto& w : j["weights"]) {
			s.weights.push_back(w.get<float>());
		}
	}
	if (j.contains("fitPoints") && j["fitPoints"].is_array()) {
		for (const auto& p : j["fitPoints"]) {
			s.fitPoints.push_back(vec3FromJson(p));
		}
	}
	reg.emplace_or_replace<ecs::CSpline3d>(e, std::move(s));
	return true;
}

bool serializeNurbsSurface(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CNurbsSurface>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CNurbsSurface>(e);
	j["degreeU"] = s.degreeU;
	j["degreeV"] = s.degreeV;
	j["countU"] = s.countU;
	j["countV"] = s.countV;
	j["controlPoints"] = ordered_json::array();
	for (const auto& p : s.controlPoints) {
		j["controlPoints"].push_back(vec3ToJson(p));
	}
	j["knotsU"] = s.knotsU;
	j["knotsV"] = s.knotsV;
	if (!s.weights.empty()) {
		j["weights"] = s.weights;
	}
	if (s.closedU) {
		j["closedU"] = true;
	}
	if (s.closedV) {
		j["closedV"] = true;
	}
	return true;
}

bool deserializeNurbsSurface(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CNurbsSurface s;
	s.degreeU = j.value("degreeU", s.degreeU);
	s.degreeV = j.value("degreeV", s.degreeV);
	s.countU = j.value("countU", s.countU);
	s.countV = j.value("countV", s.countV);
	s.closedU = j.value("closedU", s.closedU);
	s.closedV = j.value("closedV", s.closedV);
	if (j.contains("controlPoints") && j["controlPoints"].is_array()) {
		for (const auto& p : j["controlPoints"]) {
			s.controlPoints.push_back(vec3FromJson(p));
		}
	}
	if (j.contains("knotsU") && j["knotsU"].is_array()) {
		for (const auto& k : j["knotsU"]) {
			s.knotsU.push_back(k.get<float>());
		}
	}
	if (j.contains("knotsV") && j["knotsV"].is_array()) {
		for (const auto& k : j["knotsV"]) {
			s.knotsV.push_back(k.get<float>());
		}
	}
	if (j.contains("weights") && j["weights"].is_array()) {
		for (const auto& w : j["weights"]) {
			s.weights.push_back(w.get<float>());
		}
	}
	reg.emplace_or_replace<ecs::CNurbsSurface>(e, std::move(s));
	return true;
}

void writeMeshEdges(ordered_json& j, const std::vector<ecs::MeshEdge>& edges) {
	if (edges.empty()) {
		return;
	}
	j["edges"] = ordered_json::array();
	for (const auto& e : edges) {
		const ecs::MeshEdge n = ecs::meshEdge(e.a, e.b);
		j["edges"].push_back(ordered_json{{"a", n.a}, {"b", n.b}});
	}
}

void readMeshEdges(const ordered_json& j, std::vector<ecs::MeshEdge>& edges) {
	edges.clear();
	if (!j.contains("edges") || !j["edges"].is_array()) {
		return;
	}
	for (const auto& item : j["edges"]) {
		if (!item.is_object()) {
			continue;
		}
		edges.push_back(ecs::meshEdge(item.value("a", 0u), item.value("b", 0u)));
	}
}

bool serializeRing(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CRing>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CRing>(e);
	j["cx"] = s.cx;
	j["cy"] = s.cy;
	j["outerRadius"] = s.outerRadius;
	j["innerRadius"] = s.innerRadius;
	return true;
}

bool deserializeRing(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CRing s;
	s.cx = j.value("cx", s.cx);
	s.cy = j.value("cy", s.cy);
	s.outerRadius = j.value("outerRadius", s.outerRadius);
	s.innerRadius = j.value("innerRadius", s.innerRadius);
	reg.emplace_or_replace<ecs::CRing>(e, s);
	return true;
}

const char* meshModeName(ecs::CMesh::Mode mode) {
	switch (mode) {
	case ecs::CMesh::Mode::Triangles:
		return "Triangles";
	case ecs::CMesh::Mode::Lines:
		return "Lines";
	case ecs::CMesh::Mode::LineStrip:
		return "LineStrip";
	}
	return "Triangles";
}

ecs::CMesh::Mode meshModeFromName(const std::string& name) {
	if (name == "Lines") {
		return ecs::CMesh::Mode::Lines;
	}
	if (name == "LineStrip") {
		return ecs::CMesh::Mode::LineStrip;
	}
	return ecs::CMesh::Mode::Triangles;
}

bool serializeMesh(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CMesh>(e)) {
		return false;
	}
	const auto& m = reg.get<ecs::CMesh>(e);
	j["mode"] = meshModeName(m.mode);
	ordered_json positions = ordered_json::array();
	for (const auto& p : m.positions) {
		positions.push_back(vec3ToJson(p));
	}
	j["positions"] = std::move(positions);
	j["indices"] = m.indices;
	if (!m.loops.empty()) {
		j["loops"] = m.loops;
		j["loopSizes"] = m.loopSizes;
	}
	if (!m.faceColors.empty()) {
		ordered_json colors = ordered_json::array();
		for (const auto& c : m.faceColors) {
			colors.push_back(vec4ToJson(c));
		}
		j["faceColors"] = std::move(colors);
	}
	if (!m.facePalette.empty()) {
		j["facePalette"] = m.facePalette;
	}
	if (!m.texcoords.empty()) {
		ordered_json uvs = ordered_json::array();
		for (const auto& t : m.texcoords) {
			uvs.push_back(vec2ToJson(t));
		}
		j["texcoords"] = std::move(uvs);
	}
	return true;
}

bool deserializeMesh(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CMesh m;
	m.mode = meshModeFromName(j.value("mode", std::string("Triangles")));
	if (j.contains("positions") && j["positions"].is_array()) {
		for (const auto& p : j["positions"]) {
			m.positions.push_back(vec3FromJson(p));
		}
	}
	if (j.contains("indices") && j["indices"].is_array()) {
		m.indices = j["indices"].get<std::vector<uint32_t>>();
	}
	if (j.contains("loops") && j["loops"].is_array()) {
		m.loops = j["loops"].get<std::vector<uint32_t>>();
	}
	if (j.contains("loopSizes") && j["loopSizes"].is_array()) {
		m.loopSizes = j["loopSizes"].get<std::vector<uint32_t>>();
	}
	if (j.contains("faceColors") && j["faceColors"].is_array()) {
		for (const auto& c : j["faceColors"]) {
			if (c.is_array() && c.size() >= 3) {
				m.faceColors.push_back(rgbaFromJson(c));
			}
		}
	}
	if (j.contains("facePalette") && j["facePalette"].is_array()) {
		m.facePalette = j["facePalette"].get<std::vector<uint8_t>>();
	}
	if (j.contains("texcoords") && j["texcoords"].is_array()) {
		for (const auto& t : j["texcoords"]) {
			m.texcoords.push_back(vec2FromJson(t));
		}
	}
	if (!m.loopSizes.empty()) {
		ecs::meshTriangulate(m);
	}
	reg.emplace_or_replace<ecs::CMesh>(e, m);
	return true;
}

bool serializeCamera(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CCamera>(e)) {
		return false;
	}
	const auto& c = reg.get<ecs::CCamera>(e);
	j["active"] = c.active;
	j["projection"] =
		(c.projection == ecs::CCamera::Projection::Orthographic) ? "orthographic" : "perspective";
	j["fovYDegrees"] = c.fovYDegrees;
	j["orthoHeight"] = c.orthoHeight;
	j["nearClip"] = c.nearClip;
	j["farClip"] = c.farClip;
	j["aspect"] = c.aspect;
	return true;
}

bool deserializeCamera(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CCamera c;
	c.active = j.value("active", c.active);
	c.projection = (j.value("projection", std::string("perspective")) == "orthographic")
					   ? ecs::CCamera::Projection::Orthographic
					   : ecs::CCamera::Projection::Perspective;
	c.fovYDegrees = j.value("fovYDegrees", c.fovYDegrees);
	c.orthoHeight = j.value("orthoHeight", c.orthoHeight);
	c.nearClip = j.value("nearClip", c.nearClip);
	c.farClip = j.value("farClip", c.farClip);
	c.aspect = j.value("aspect", c.aspect);
	reg.emplace_or_replace<ecs::CCamera>(e, c);
	return true;
}

bool serializeLight(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CLight>(e)) {
		return false;
	}
	const auto& l = reg.get<ecs::CLight>(e);
	j["enabled"] = l.enabled;
	j["type"] = (l.type == ecs::CLight::Type::Point) ? "point" : "directional";
	j["rgb"] = rgbToJson({l.colorR, l.colorG, l.colorB});
	j["intensity"] = l.intensity;
	j["ambient"] = l.ambient;
	j["banded"] = l.banded;
	j["bands"] = l.bands;
	return true;
}

bool deserializeLight(entt::registry& reg, entt::entity e, const ordered_json& j) {
	auto& l = reg.get_or_emplace<ecs::CLight>(e);
	l.enabled = j.value("enabled", l.enabled);
	l.type = (j.value("type", std::string("directional")) == "point")
				 ? ecs::CLight::Type::Point
				 : ecs::CLight::Type::Directional;
	if (j.contains("rgb")) {
		const glm::vec3 rgb = rgbFromJson(j["rgb"], {l.colorR, l.colorG, l.colorB});
		l.colorR = rgb.r;
		l.colorG = rgb.g;
		l.colorB = rgb.b;
	}
	l.intensity = j.value("intensity", l.intensity);
	l.ambient = j.value("ambient", l.ambient);
	l.banded = j.value("banded", l.banded);
	l.bands = j.value("bands", l.bands);
	return true;
}

/// Dither is a Kit shading choice with no Contract field, and the light schema
/// admits no extras, so it travels beside the light rather than inside it.
bool serializeLightShading(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CLight>(e)) {
		return false;
	}
	const auto& l = reg.get<ecs::CLight>(e);
	if (!l.dither) {
		return false;
	}
	j["dither"] = l.dither;
	return true;
}

bool deserializeLightShading(entt::registry& reg, entt::entity e, const ordered_json& j) {
	auto& l = reg.get_or_emplace<ecs::CLight>(e);
	l.dither = j.value("dither", l.dither);
	return true;
}

bool serializeEnvironment(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CRenderEnvironment>(e)) {
		return false;
	}
	const auto& env = reg.get<ecs::CRenderEnvironment>(e);
	if (env.hdri == entt::null) {
		j["hdri"] = nullptr;
	} else {
		j["hdri"] = entityIdString(env.hdri);
	}
	if (env.intensity != 1.f) {
		j["intensity"] = env.intensity;
	}
	return true;
}

bool deserializeEnvironment(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CRenderEnvironment env;
	if (j.contains("hdri") && j["hdri"].is_string()) {
		env.hdri = entityIdFromString(j["hdri"].get<std::string>());
	}
	env.intensity = j.value("intensity", 1.f);
	reg.emplace_or_replace<ecs::CRenderEnvironment>(e, env);
	return true;
}

void remapEnvironment(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CRenderEnvironment>()) {
		auto& env = reg.get<ecs::CRenderEnvironment>(entity);
		if (env.hdri == entt::null) {
			continue;
		}
		const auto it = idMap.find(static_cast<std::uint32_t>(env.hdri));
		env.hdri = (it != idMap.end()) ? it->second : entt::null;
	}
}

bool serializePalette(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPalette>(e)) {
		return false;
	}
	const auto& p = reg.get<ecs::CPalette>(e);
	ordered_json colors = ordered_json::array();
	for (int i = 0; i < ecs::CPalette::kCount; ++i) {
		colors.push_back(vec4ToJson(p.colors[static_cast<size_t>(i)]));
	}
	j["colors"] = std::move(colors);
	return true;
}

bool deserializePalette(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPalette p =
		reg.all_of<ecs::CPalette>(e) ? reg.get<ecs::CPalette>(e) : ecs::CPalette::default16();
	if (j.contains("colors") && j["colors"].is_array()) {
		const size_t n = std::min(j["colors"].size(), static_cast<size_t>(ecs::CPalette::kCount));
		for (size_t i = 0; i < n; ++i) {
			p.colors[i] = vec4FromJson(j["colors"][i], p.colors[i]);
		}
	}
	reg.emplace_or_replace<ecs::CPalette>(e, p);
	return true;
}

bool serializePaletteShade(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPalette>(e)) {
		return false;
	}
	const auto& p = reg.get<ecs::CPalette>(e);
	ordered_json next = ordered_json::array();
	for (int i = 0; i < ecs::CPalette::kCount; ++i) {
		next.push_back(p.shadeNext[static_cast<size_t>(i)]);
	}
	j["shadeNext"] = std::move(next);
	return true;
}

bool deserializePaletteShade(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPalette p =
		reg.all_of<ecs::CPalette>(e) ? reg.get<ecs::CPalette>(e) : ecs::CPalette::default16();
	if (j.contains("shadeNext") && j["shadeNext"].is_array()) {
		const size_t n =
			std::min(j["shadeNext"].size(), static_cast<size_t>(ecs::CPalette::kCount));
		for (size_t i = 0; i < n; ++i) {
			p.shadeNext[i] = j["shadeNext"][i].get<int>();
		}
	}
	reg.emplace_or_replace<ecs::CPalette>(e, p);
	return true;
}

bool serializeIndexedAtlas(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CIndexedAtlas>(e)) {
		return false;
	}
	const auto& a = reg.get<ecs::CIndexedAtlas>(e);
	j["width"] = a.width;
	j["height"] = a.height;
	j["indices"] = a.indices;
	return true;
}

bool deserializeIndexedAtlas(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CIndexedAtlas a;
	a.width = j.value("width", a.width);
	a.height = j.value("height", a.height);
	if (j.contains("indices") && j["indices"].is_array()) {
		a.indices = j["indices"].get<std::vector<uint8_t>>();
	} else {
		a.resize(a.width, a.height, 0);
	}
	reg.emplace_or_replace<ecs::CIndexedAtlas>(e, a);
	return true;
}

bool serializeFaceSelection(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFaceSelection>(e)) {
		return false;
	}
	j["faces"] = reg.get<ecs::CFaceSelection>(e).faces;
	return true;
}

bool deserializeFaceSelection(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFaceSelection s;
	if (j.contains("faces") && j["faces"].is_array()) {
		s.faces = j["faces"].get<std::vector<uint32_t>>();
	}
	reg.emplace_or_replace<ecs::CFaceSelection>(e, s);
	return true;
}

bool serializeEdgeSelection(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CEdgeSelection>(e)) {
		return false;
	}
	writeMeshEdges(j, reg.get<ecs::CEdgeSelection>(e).edges);
	return true;
}

bool deserializeEdgeSelection(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CEdgeSelection s;
	readMeshEdges(j, s.edges);
	reg.emplace_or_replace<ecs::CEdgeSelection>(e, std::move(s));
	return true;
}

const char* strokeCapName(ecs::CDrawStyle::StrokeCap cap) {
	switch (cap) {
	case ecs::CDrawStyle::StrokeCap::Butt:
		return "Butt";
	case ecs::CDrawStyle::StrokeCap::Square:
		return "Square";
	case ecs::CDrawStyle::StrokeCap::Round:
		return "Round";
	}
	return "Butt";
}

ecs::CDrawStyle::StrokeCap strokeCapFromName(const std::string& name) {
	if (name == "Square") {
		return ecs::CDrawStyle::StrokeCap::Square;
	}
	if (name == "Round") {
		return ecs::CDrawStyle::StrokeCap::Round;
	}
	return ecs::CDrawStyle::StrokeCap::Butt;
}

const char* strokeJoinName(ecs::CDrawStyle::StrokeJoin join) {
	switch (join) {
	case ecs::CDrawStyle::StrokeJoin::Miter:
		return "Miter";
	case ecs::CDrawStyle::StrokeJoin::Bevel:
		return "Bevel";
	case ecs::CDrawStyle::StrokeJoin::Round:
		return "Round";
	}
	return "Miter";
}

ecs::CDrawStyle::StrokeJoin strokeJoinFromName(const std::string& name) {
	if (name == "Bevel") {
		return ecs::CDrawStyle::StrokeJoin::Bevel;
	}
	if (name == "Round") {
		return ecs::CDrawStyle::StrokeJoin::Round;
	}
	return ecs::CDrawStyle::StrokeJoin::Miter;
}

bool serializeDrawStyle(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CDrawStyle>(e)) {
		return false;
	}
	const auto& d = reg.get<ecs::CDrawStyle>(e);
	j["fillRgba"] = colorToJson({d.fillR, d.fillG, d.fillB, d.fillA});
	j["strokeRgba"] = colorToJson({d.strokeR, d.strokeG, d.strokeB, d.strokeA});
	j["strokeWidth"] = d.strokeWidth;
	j["hasFill"] = d.hasFill;
	j["hasStroke"] = d.hasStroke;
	return true;
}

// CDrawStyle spans two wire components, so both codecs patch the same struct
// instead of replacing it - whichever key is read second must not wipe the first.
bool deserializeDrawStyle(entt::registry& reg, entt::entity e, const ordered_json& j) {
	auto& d = reg.get_or_emplace<ecs::CDrawStyle>(e);
	if (j.contains("fillRgba")) {
		const glm::vec4 fill = rgbaFromJson(j["fillRgba"], {d.fillR, d.fillG, d.fillB, d.fillA});
		d.fillR = fill.r;
		d.fillG = fill.g;
		d.fillB = fill.b;
		d.fillA = fill.a;
	}
	if (j.contains("strokeRgba")) {
		const glm::vec4 stroke =
			rgbaFromJson(j["strokeRgba"], {d.strokeR, d.strokeG, d.strokeB, d.strokeA});
		d.strokeR = stroke.r;
		d.strokeG = stroke.g;
		d.strokeB = stroke.b;
		d.strokeA = stroke.a;
	}
	d.strokeWidth = j.value("strokeWidth", d.strokeWidth);
	d.hasFill = j.value("hasFill", d.hasFill);
	d.hasStroke = j.value("hasStroke", d.hasStroke);
	return true;
}

/// Caps / joins / dash / vector-effect. Schema is `rig.paint.stroke_style`.
bool serializeStrokeStyle(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CDrawStyle>(e)) {
		return false;
	}
	const auto& d = reg.get<ecs::CDrawStyle>(e);
	const ecs::CDrawStyle plain;
	if (d.strokeCap == plain.strokeCap && d.strokeJoin == plain.strokeJoin &&
		d.miterLimit == plain.miterLimit && d.vectorEffect == plain.vectorEffect &&
		d.dashPattern.empty() && d.dashOffset == plain.dashOffset) {
		return false;
	}
	auto cap = [](ecs::CDrawStyle::StrokeCap c) {
		return c == ecs::CDrawStyle::StrokeCap::Square	? "square"
			   : c == ecs::CDrawStyle::StrokeCap::Round ? "round"
														: "butt";
	};
	auto join = [](ecs::CDrawStyle::StrokeJoin jn) {
		return jn == ecs::CDrawStyle::StrokeJoin::Bevel	  ? "bevel"
			   : jn == ecs::CDrawStyle::StrokeJoin::Round ? "round"
														  : "miter";
	};
	j["cap"] = cap(d.strokeCap);
	j["join"] = join(d.strokeJoin);
	if (d.miterLimit != 4.f) {
		j["miterLimit"] = d.miterLimit;
	}
	if (!d.dashPattern.empty()) {
		j["dash"] = d.dashPattern;
		j["dashOffset"] = d.dashOffset;
	}
	if (d.vectorEffect == ecs::CDrawStyle::VectorEffect::NonScalingStroke) {
		j["vectorEffect"] = "non-scaling-stroke";
	}
	return true;
}

bool serializeStrokeStyleLegacy(entt::registry&, entt::entity, ordered_json&) {
	return false;
}

bool deserializeStrokeStyle(entt::registry& reg, entt::entity e, const ordered_json& j) {
	auto& d = reg.get_or_emplace<ecs::CDrawStyle>(e);
	std::string cap = j.value("cap", std::string());
	if (cap.empty()) {
		cap = j.value("strokeCap", std::string("butt"));
	}
	if (cap == "Square" || cap == "square") {
		d.strokeCap = ecs::CDrawStyle::StrokeCap::Square;
	} else if (cap == "Round" || cap == "round") {
		d.strokeCap = ecs::CDrawStyle::StrokeCap::Round;
	} else {
		d.strokeCap = ecs::CDrawStyle::StrokeCap::Butt;
	}
	std::string join = j.value("join", std::string());
	if (join.empty()) {
		join = j.value("strokeJoin", std::string("miter"));
	}
	if (join == "Bevel" || join == "bevel") {
		d.strokeJoin = ecs::CDrawStyle::StrokeJoin::Bevel;
	} else if (join == "Round" || join == "round") {
		d.strokeJoin = ecs::CDrawStyle::StrokeJoin::Round;
	} else {
		d.strokeJoin = ecs::CDrawStyle::StrokeJoin::Miter;
	}
	d.miterLimit = j.value("miterLimit", d.miterLimit);
	if (j.contains("dash") && j["dash"].is_array()) {
		d.dashPattern = j["dash"].get<std::vector<float>>();
	} else if (j.contains("dashPattern") && j["dashPattern"].is_array()) {
		d.dashPattern = j["dashPattern"].get<std::vector<float>>();
	}
	d.dashOffset = j.value("dashOffset", d.dashOffset);
	d.vectorEffect = j.value("vectorEffect", std::string("none")) == "non-scaling-stroke"
						 ? ecs::CDrawStyle::VectorEffect::NonScalingStroke
						 : ecs::CDrawStyle::VectorEffect::None;
	return true;
}

bool serializeRelationship(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CRelationship>(e)) {
		return false;
	}
	const auto& r = reg.get<ecs::CRelationship>(e);
	if (r.parent == entt::null) {
		j["parent"] = nullptr;
	} else {
		j["parent"] = entityIdString(r.parent);
	}
	if (r.order != 0) {
		j["order"] = r.order;
	}
	return true;
}

bool deserializeRelationship(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CRelationship r;
	// Parks the saved handle here; the load pass remaps it to the live entity
	// once every id in the document has been seen.
	if (j.contains("parent") && j["parent"].is_string()) {
		r.parent = entityIdFromString(j["parent"].get<std::string>());
	} else {
		r.parent = entt::null;
	}
	r.order = j.value("order", 0);
	reg.emplace_or_replace<ecs::CRelationship>(e, r);
	return true;
}

bool serializeGuide(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CGuide>(e)) {
		return false;
	}
	const auto& g = reg.get<ecs::CGuide>(e);
	j["position"] = g.position;
	j["vertical"] = g.vertical;
	j["color"] = vec4ToJson(g.color);
	return true;
}

bool deserializeGuide(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CGuide g{};
	g.position = j.value("position", 0.0f);
	g.vertical = j.value("vertical", false);
	if (j.contains("color")) {
		g.color = vec4FromJson(j["color"], g.color);
	}
	reg.emplace_or_replace<ecs::CGuide>(e, g);
	return true;
}

glm::vec2 pathPointFrom(const ordered_json& cmd, const char* schemaKey, glm::vec2 fallback = {}) {
	if (cmd.contains(schemaKey)) {
		return vec2FromJson(cmd[schemaKey], fallback);
	}
	return fallback;
}

const char* pathCmdType(ecs::CPath::Cmd t) {
	switch (t) {
	case ecs::CPath::Cmd::MoveTo:
		return "move-to";
	case ecs::CPath::Cmd::LineTo:
		return "line-to";
	case ecs::CPath::Cmd::CubicTo:
		return "cubic-to";
	case ecs::CPath::Cmd::QuadTo:
		return "quad-to";
	case ecs::CPath::Cmd::Close:
		return "close";
	case ecs::CPath::Cmd::ArcTo:
		return "arc-to";
	}
	return "move-to";
}

bool serializePath(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPath>(e)) {
		return false;
	}
	const auto& path = reg.get<ecs::CPath>(e);
	auto commands = ordered_json::array();
	auto emit = [&](const ecs::CPath::Command& cmd) {
		ordered_json row;
		row["type"] = pathCmdType(cmd.type);
		switch (cmd.type) {
		case ecs::CPath::Cmd::MoveTo:
		case ecs::CPath::Cmd::LineTo:
			row["point"] = vec2ToJson(cmd.p);
			break;
		case ecs::CPath::Cmd::CubicTo:
			row["point"] = vec2ToJson(cmd.p);
			row["control1"] = vec2ToJson(cmd.c1);
			row["control2"] = vec2ToJson(cmd.c2);
			break;
		case ecs::CPath::Cmd::QuadTo:
			row["point"] = vec2ToJson(cmd.p);
			row["control1"] = vec2ToJson(cmd.c1);
			break;
		case ecs::CPath::Cmd::Close:
			break;
		case ecs::CPath::Cmd::ArcTo:
			row["point"] = vec2ToJson(cmd.p);
			row["radiusX"] = cmd.radii.x;
			row["radiusY"] = cmd.radii.y;
			row["xAxisRotationDegrees"] = cmd.xRotDeg;
			if (cmd.largeArc) {
				row["largeArc"] = true;
			}
			if (cmd.sweep) {
				row["sweep"] = true;
			}
			break;
		}
		commands.push_back(std::move(row));
	};
	for (const auto& cmd : path.commands) {
		emit(cmd);
	}
	j["commands"] = std::move(commands);
	if (path.evenOdd) {
		j["fillRule"] = "evenodd";
	}
	return true;
}

bool deserializePath(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPath path;
	if (j.contains("commands") && j["commands"].is_array()) {
		for (const auto& cmd : j["commands"]) {
			const std::string type = cmd.value("type", std::string("move-to"));
			ecs::CPath::Command row;
			if (type == "line-to") {
				row.type = ecs::CPath::Cmd::LineTo;
				row.p = pathPointFrom(cmd, "point");
			} else if (type == "cubic-to") {
				row.type = ecs::CPath::Cmd::CubicTo;
				row.p = pathPointFrom(cmd, "point");
				row.c1 = pathPointFrom(cmd, "control1");
				row.c2 = pathPointFrom(cmd, "control2");
			} else if (type == "quad-to") {
				row.type = ecs::CPath::Cmd::QuadTo;
				row.p = pathPointFrom(cmd, "point");
				row.c1 = pathPointFrom(cmd, "control1");
			} else if (type == "close") {
				row.type = ecs::CPath::Cmd::Close;
			} else if (type == "arc-to") {
				row.type = ecs::CPath::Cmd::ArcTo;
				row.p = pathPointFrom(cmd, "point");
				row.radii.x = cmd.value("radiusX", 0.f);
				row.radii.y = cmd.value("radiusY", 0.f);
				row.xRotDeg = cmd.value("xAxisRotationDegrees", 0.f);
				row.largeArc = cmd.value("largeArc", false);
				row.sweep = cmd.value("sweep", false);
			} else {
				row.type = ecs::CPath::Cmd::MoveTo;
				row.p = pathPointFrom(cmd, "point");
			}
			path.commands.push_back(row);
		}
	}
	path.evenOdd = j.value("fillRule", std::string("nonzero")) == "evenodd";
	reg.emplace_or_replace<ecs::CPath>(e, std::move(path));
	return true;
}

bool serializeLayer(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CLayer>(e)) {
		return false;
	}
	const auto& l = reg.get<ecs::CLayer>(e);
	j["order"] = l.order;
	j["locked"] = l.locked;
	j["rgba"] = colorToJson({l.colorR, l.colorG, l.colorB, l.colorA});
	return true;
}

bool deserializeLayer(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CLayer l;
	if (reg.all_of<ecs::CLayer>(e)) {
		l = reg.get<ecs::CLayer>(e);
	}
	l.order = j.value("order", l.order);
	l.locked = j.value("locked", l.locked);
	if (j.contains("rgba")) {
		const glm::vec4 rgba = rgbaFromJson(j["rgba"], {l.colorR, l.colorG, l.colorB, l.colorA});
		l.colorR = rgba.r;
		l.colorG = rgba.g;
		l.colorB = rgba.b;
		l.colorA = rgba.a;
	}
	reg.emplace_or_replace<ecs::CLayer>(e, l);
	return true;
}

bool serializeLayerVisible(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CLayer>(e)) {
		return false;
	}
	if (reg.all_of<ecs::CRenderVisibility>(e)) {
		return false; // Contract hide bit lives on rig.render.visibility
	}
	const auto& l = reg.get<ecs::CLayer>(e);
	if (l.visible) {
		return false; // default on the wire
	}
	j["visible"] = false;
	return true;
}

bool serializeRenderVisibility(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CRenderVisibility>(e)) {
		return false;
	}
	const auto& v = reg.get<ecs::CRenderVisibility>(e);
	if (v.visible) {
		return false;
	}
	j["visible"] = false;
	return true;
}

bool deserializeRenderVisibility(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CRenderVisibility v;
	if (reg.all_of<ecs::CRenderVisibility>(e)) {
		v = reg.get<ecs::CRenderVisibility>(e);
	}
	v.visible = j.value("visible", v.visible);
	reg.emplace_or_replace<ecs::CRenderVisibility>(e, v);
	if (reg.all_of<ecs::CLayer>(e)) {
		reg.get<ecs::CLayer>(e).visible = v.visible;
	}
	return true;
}

bool serializeRenderBlend(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CRenderBlend>(e)) {
		return false;
	}
	const auto& b = reg.get<ecs::CRenderBlend>(e);
	const bool defaultMode = b.blendMode == 0;
	const bool defaultOp = b.opacity >= 0.999f;
	if (defaultMode && defaultOp) {
		return false;
	}
	if (!defaultMode) {
		j["blendMode"] = ecs::renderBlendToken(b.blendMode);
	}
	if (!defaultOp) {
		j["opacity"] = b.opacity;
	}
	return true;
}

bool deserializeRenderBlend(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CRenderBlend b;
	if (reg.all_of<ecs::CRenderBlend>(e)) {
		b = reg.get<ecs::CRenderBlend>(e);
	}
	if (j.contains("blendMode")) {
		if (j["blendMode"].is_string()) {
			b.blendMode = ecs::renderBlendFromToken(j["blendMode"].get<std::string>());
		} else if (j["blendMode"].is_number_integer()) {
			b.blendMode = j["blendMode"].get<int>();
		}
	}
	b.opacity = j.value("opacity", b.opacity);
	reg.emplace_or_replace<ecs::CRenderBlend>(e, b);
	return true;
}

bool serializeUiPanel(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CUiPanel>(e)) {
		return false;
	}
	const auto& p = reg.get<ecs::CUiPanel>(e);
	if (!p.role.empty()) {
		j["role"] = p.role;
	}
	j["order"] = p.order;
	j["visible"] = p.visible;
	j["preferredWidth"] = p.preferredWidth;
	j["preferredHeight"] = p.preferredHeight;
	return true;
}

bool deserializeUiPanel(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CUiPanel p;
	p.role = j.value("role", p.role);
	p.order = j.value("order", p.order);
	p.visible = j.value("visible", p.visible);
	p.preferredWidth = j.value("preferredWidth", p.preferredWidth);
	p.preferredHeight = j.value("preferredHeight", p.preferredHeight);
	reg.emplace_or_replace<ecs::CUiPanel>(e, std::move(p));
	return true;
}

bool serializeUiGroup(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CUiGroup>(e)) {
		return false;
	}
	const auto& g = reg.get<ecs::CUiGroup>(e);
	if (g.panel != 0) {
		j["panel"] = entityIdString(static_cast<entt::entity>(g.panel));
	}
	if (g.parent != 0) {
		j["parent"] = entityIdString(static_cast<entt::entity>(g.parent));
	}
	j["order"] = g.order;
	j["orientation"] = g.orientation;
	if (g.collapsed) {
		j["collapsed"] = true;
	}
	return true;
}

bool deserializeUiGroup(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CUiGroup g;
	if (j.contains("panel") && j["panel"].is_string()) {
		g.panel = static_cast<uint32_t>(entityIdFromString(j["panel"].get<std::string>()));
	}
	if (j.contains("parent") && j["parent"].is_string()) {
		g.parent = static_cast<uint32_t>(entityIdFromString(j["parent"].get<std::string>()));
	}
	g.order = j.value("order", g.order);
	g.orientation = j.value("orientation", g.orientation);
	g.collapsed = j.value("collapsed", g.collapsed);
	reg.emplace_or_replace<ecs::CUiGroup>(e, std::move(g));
	return true;
}

bool serializeUiControl(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CUiControl>(e)) {
		return false;
	}
	const auto& c = reg.get<ecs::CUiControl>(e);
	j["panel"] = entityIdString(static_cast<entt::entity>(c.panel));
	if (c.group != 0) {
		j["group"] = entityIdString(static_cast<entt::entity>(c.group));
	}
	j["order"] = c.order;
	j["target"] = entityIdString(static_cast<entt::entity>(c.target));
	j["propertyKey"] = c.propertyKey;
	j["type"] = c.type;
	j["enabled"] = c.enabled;
	j["readOnly"] = c.readOnly;
	if (c.widget != "auto") {
		j["widget"] = c.widget;
	}
	if (c.min != 0.f) {
		j["min"] = c.min;
	}
	if (c.max != 1.f) {
		j["max"] = c.max;
	}
	if (c.step != 0.01f) {
		j["step"] = c.step;
	}
	return true;
}

bool deserializeUiControl(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CUiControl c;
	if (j.contains("panel") && j["panel"].is_string()) {
		c.panel = static_cast<uint32_t>(entityIdFromString(j["panel"].get<std::string>()));
	}
	if (j.contains("group") && j["group"].is_string()) {
		c.group = static_cast<uint32_t>(entityIdFromString(j["group"].get<std::string>()));
	}
	if (j.contains("target") && j["target"].is_string()) {
		c.target = static_cast<uint32_t>(entityIdFromString(j["target"].get<std::string>()));
	}
	c.order = j.value("order", c.order);
	c.propertyKey = j.value("propertyKey", c.propertyKey);
	c.type = j.value("type", c.type);
	c.enabled = j.value("enabled", c.enabled);
	c.readOnly = j.value("readOnly", c.readOnly);
	c.widget = j.value("widget", c.widget);
	c.min = j.value("min", c.min);
	c.max = j.value("max", c.max);
	c.step = j.value("step", c.step);
	reg.emplace_or_replace<ecs::CUiControl>(e, std::move(c));
	return true;
}

void remapUiRefs(entt::registry& reg, const EntityIdMap& idMap) {
	auto remap = [&](uint32_t& id) {
		if (id == 0) {
			return;
		}
		const auto it = idMap.find(id);
		id = (it != idMap.end()) ? static_cast<uint32_t>(it->second) : 0;
	};
	for (auto e : reg.view<ecs::CUiGroup>()) {
		auto& g = reg.get<ecs::CUiGroup>(e);
		remap(g.panel);
		remap(g.parent);
	}
	for (auto e : reg.view<ecs::CUiControl>()) {
		auto& c = reg.get<ecs::CUiControl>(e);
		remap(c.panel);
		remap(c.group);
		remap(c.target);
	}
}

bool deserializeLayerVisible(entt::registry& reg, entt::entity e, const ordered_json& j) {
	auto& l = reg.get_or_emplace<ecs::CLayer>(e);
	l.visible = j.value("visible", l.visible);
	return true;
}

const char* assetKindName(ecs::CAssetRef::Kind k) {
	switch (k) {
	case ecs::CAssetRef::Kind::Image:
		return "image";
	case ecs::CAssetRef::Kind::Audio:
		return "audio";
	case ecs::CAssetRef::Kind::Video:
		return "video";
	case ecs::CAssetRef::Kind::Model:
		return "model";
	case ecs::CAssetRef::Kind::Font:
		return "font";
	case ecs::CAssetRef::Kind::Document:
		return "document";
	case ecs::CAssetRef::Kind::Preset:
		return "preset";
	case ecs::CAssetRef::Kind::Other:
		return "other";
	}
	return "other";
}

ecs::CAssetRef::Kind assetKindFromName(const std::string& name) {
	if (name == "image") {
		return ecs::CAssetRef::Kind::Image;
	}
	if (name == "audio") {
		return ecs::CAssetRef::Kind::Audio;
	}
	if (name == "video") {
		return ecs::CAssetRef::Kind::Video;
	}
	if (name == "model") {
		return ecs::CAssetRef::Kind::Model;
	}
	if (name == "font") {
		return ecs::CAssetRef::Kind::Font;
	}
	if (name == "document") {
		return ecs::CAssetRef::Kind::Document;
	}
	if (name == "preset") {
		return ecs::CAssetRef::Kind::Preset;
	}
	return ecs::CAssetRef::Kind::Other;
}

bool serializeAssetRef(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CAssetRef>(e)) {
		return false;
	}
	const auto& a = reg.get<ecs::CAssetRef>(e);
	j["kind"] = assetKindName(a.kind);
	j["path"] = a.path;
	if (a.loop) {
		j["loop"] = true;
	}
	return true;
}

bool deserializeAssetRef(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CAssetRef a;
	a.kind = assetKindFromName(j.value("kind", std::string("other")));
	a.path = j.value("path", a.path);
	a.loop = j.value("loop", a.loop);
	reg.emplace_or_replace<ecs::CAssetRef>(e, std::move(a));
	return true;
}

bool serializeText(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CText>(e)) {
		return false;
	}
	const auto& t = reg.get<ecs::CText>(e);
	j["text"] = t.text;
	if (t.font != entt::null) {
		j["font"] = entityIdString(t.font);
	}
	j["fontSize"] = t.fontSize;
	if (!t.axes.empty()) {
		auto axes = ordered_json::array();
		for (const auto& ax : t.axes) {
			axes.push_back(ordered_json{{"tag", ax.tag}, {"value", ax.value}});
		}
		j["axes"] = std::move(axes);
	}
	if (!t.features.empty()) {
		j["features"] = t.features;
	}
	if (!t.useKerning) {
		j["useKerning"] = false;
	}
	if (t.tracking != 0.f) {
		j["tracking"] = t.tracking;
	}
	if (t.anchor != ecs::CText::Anchor::Start) {
		j["anchor"] = t.anchor == ecs::CText::Anchor::Middle ? "middle" : "end";
	}
	if (t.writingMode != ecs::CText::WritingMode::HorizontalTb) {
		j["writingMode"] = t.writingMode == ecs::CText::WritingMode::VerticalRl ? "vertical-rl"
																			   : "vertical-lr";
	}
	if (t.direction == ecs::CText::Direction::Rtl) {
		j["direction"] = "rtl";
	}
	if (t.baseline != ecs::CText::Baseline::Auto) {
		const char* b = "auto";
		switch (t.baseline) {
		case ecs::CText::Baseline::Alphabetic:
			b = "alphabetic";
			break;
		case ecs::CText::Baseline::Middle:
			b = "middle";
			break;
		case ecs::CText::Baseline::Hanging:
			b = "hanging";
			break;
		case ecs::CText::Baseline::Ideographic:
			b = "ideographic";
			break;
		case ecs::CText::Baseline::Central:
			b = "central";
			break;
		default:
			break;
		}
		j["baseline"] = b;
	}
	if (!t.placements.empty()) {
		auto rows = ordered_json::array();
		for (const auto& p : t.placements) {
			ordered_json row;
			row["index"] = p.index;
			if (p.hasX) {
				row["x"] = p.x;
			}
			if (p.hasY) {
				row["y"] = p.y;
			}
			if (p.dx != 0.f) {
				row["dx"] = p.dx;
			}
			if (p.dy != 0.f) {
				row["dy"] = p.dy;
			}
			if (p.rotateDegrees != 0.f) {
				row["rotateDegrees"] = p.rotateDegrees;
			}
			rows.push_back(std::move(row));
		}
		j["placements"] = std::move(rows);
	}
	return true;
}

bool deserializeText(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CText t;
	t.text = j.value("text", t.text);
	if (j.contains("font") && j["font"].is_string()) {
		t.font = entityIdFromString(j["font"].get<std::string>());
	} else {
		t.font = entt::null;
	}
	t.fontSize = j.value("fontSize", t.fontSize);
	if (j.contains("axes") && j["axes"].is_array()) {
		for (const auto& ax : j["axes"]) {
			ecs::FontAxisValue row;
			row.tag = ax.value("tag", 0u);
			row.value = ax.value("value", 0.f);
			t.axes.push_back(row);
		}
	}
	t.features = j.value("features", t.features);
	t.useKerning = j.value("useKerning", t.useKerning);
	t.tracking = j.value("tracking", t.tracking);
	{
		const std::string a = j.value("anchor", std::string("start"));
		t.anchor = a == "middle"   ? ecs::CText::Anchor::Middle
				   : a == "end"	   ? ecs::CText::Anchor::End
								   : ecs::CText::Anchor::Start;
		const std::string wm = j.value("writingMode", std::string("horizontal-tb"));
		t.writingMode = wm == "vertical-rl"	  ? ecs::CText::WritingMode::VerticalRl
						: wm == "vertical-lr" ? ecs::CText::WritingMode::VerticalLr
											  : ecs::CText::WritingMode::HorizontalTb;
		t.direction = j.value("direction", std::string("ltr")) == "rtl"
						  ? ecs::CText::Direction::Rtl
						  : ecs::CText::Direction::Ltr;
		const std::string b = j.value("baseline", std::string("auto"));
		t.baseline = b == "alphabetic"	   ? ecs::CText::Baseline::Alphabetic
					 : b == "middle"	   ? ecs::CText::Baseline::Middle
					 : b == "hanging"	   ? ecs::CText::Baseline::Hanging
					 : b == "ideographic"  ? ecs::CText::Baseline::Ideographic
					 : b == "central"	   ? ecs::CText::Baseline::Central
										   : ecs::CText::Baseline::Auto;
	}
	if (j.contains("placements") && j["placements"].is_array()) {
		for (const auto& row : j["placements"]) {
			ecs::CText::Placement p;
			p.index = row.value("index", 0);
			if (row.contains("x")) {
				p.hasX = true;
				p.x = row.value("x", 0.f);
			}
			if (row.contains("y")) {
				p.hasY = true;
				p.y = row.value("y", 0.f);
			}
			p.dx = row.value("dx", 0.f);
			p.dy = row.value("dy", 0.f);
			p.rotateDegrees = row.value("rotateDegrees", 0.f);
			t.placements.push_back(p);
		}
	}
	reg.emplace_or_replace<ecs::CText>(e, std::move(t));
	return true;
}

void remapEntityField(entt::entity& e, const EntityIdMap& idMap) {
	if (e == entt::null) {
		return;
	}
	const auto it = idMap.find(static_cast<std::uint32_t>(e));
	e = (it != idMap.end()) ? it->second : entt::null;
}

void remapTextFont(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CText>()) {
		auto& t = reg.get<ecs::CText>(entity);
		remapEntityField(t.font, idMap);
	}
}

void remapTextPath(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CTextPath>()) {
		remapEntityField(reg.get<ecs::CTextPath>(entity).path, idMap);
	}
}

void remapFontFace(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CFontFace>()) {
		remapEntityField(reg.get<ecs::CFontFace>(entity).asset, idMap);
	}
}

void remapInstance(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CInstance>()) {
		remapEntityField(reg.get<ecs::CInstance>(entity).source, idMap);
	}
}

void remapPaintClip(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CPaintClip>()) {
		remapEntityField(reg.get<ecs::CPaintClip>(entity).path, idMap);
	}
}

void remapPaintMask(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CPaintMask>()) {
		remapEntityField(reg.get<ecs::CPaintMask>(entity).source, idMap);
	}
}

void remapPaintMarkers(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CPaintMarkers>()) {
		auto& m = reg.get<ecs::CPaintMarkers>(entity);
		remapEntityField(m.start, idMap);
		remapEntityField(m.mid, idMap);
		remapEntityField(m.end, idMap);
	}
}

void remapPaintFilter(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CPaintFilter>()) {
		remapEntityField(reg.get<ecs::CPaintFilter>(entity).filter, idMap);
	}
}

void remapPaintFilterStep(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto entity : reg.view<ecs::CPaintFilterStep>()) {
		remapEntityField(reg.get<ecs::CPaintFilterStep>(entity).asset, idMap);
	}
}

bool serializeEntityRef(ordered_json& j, const char* key, entt::entity e) {
	if (e == entt::null) {
		return false;
	}
	j[key] = entityIdString(e);
	return true;
}

entt::entity readEntityRef(const ordered_json& j, const char* key) {
	if (!j.contains(key) || !j[key].is_string()) {
		return entt::null;
	}
	return entityIdFromString(j[key].get<std::string>());
}

bool serializeTextPath(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CTextPath>(e)) {
		return false;
	}
	const auto& t = reg.get<ecs::CTextPath>(e);
	if (t.path == entt::null) {
		return false;
	}
	j["path"] = entityIdString(t.path);
	if (t.startOffset != 0.f) {
		j["startOffset"] = t.startOffset;
	}
	if (t.startOffsetMode == ecs::CTextPath::OffsetMode::Ratio) {
		j["startOffsetMode"] = "ratio";
	}
	if (t.method == ecs::CTextPath::Method::Stretch) {
		j["method"] = "stretch";
	}
	if (t.spacing == ecs::CTextPath::Spacing::Auto) {
		j["spacing"] = "auto";
	}
	if (t.side == ecs::CTextPath::Side::Right) {
		j["side"] = "right";
	}
	return true;
}

bool deserializeTextPath(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CTextPath t;
	t.path = readEntityRef(j, "path");
	t.startOffset = j.value("startOffset", 0.f);
	t.startOffsetMode = j.value("startOffsetMode", std::string("length")) == "ratio"
							? ecs::CTextPath::OffsetMode::Ratio
							: ecs::CTextPath::OffsetMode::Length;
	t.method = j.value("method", std::string("align")) == "stretch"
				   ? ecs::CTextPath::Method::Stretch
				   : ecs::CTextPath::Method::Align;
	t.spacing = j.value("spacing", std::string("exact")) == "auto"
					? ecs::CTextPath::Spacing::Auto
					: ecs::CTextPath::Spacing::Exact;
	t.side = j.value("side", std::string("left")) == "right" ? ecs::CTextPath::Side::Right
															: ecs::CTextPath::Side::Left;
	reg.emplace_or_replace<ecs::CTextPath>(e, t);
	return true;
}

bool serializeFontFace(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontFace>(e)) {
		return false;
	}
	const auto& f = reg.get<ecs::CFontFace>(e);
	j["family"] = f.family;
	if (f.asset != entt::null) {
		j["asset"] = entityIdString(f.asset);
	}
	if (f.weight != 400.f) {
		j["weight"] = f.weight;
	}
	if (f.style == ecs::CFontFace::Style::Italic) {
		j["style"] = "italic";
	}
	return true;
}

bool deserializeFontFace(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontFace f;
	f.family = j.value("family", f.family);
	f.asset = readEntityRef(j, "asset");
	f.weight = j.value("weight", f.weight);
	f.style = j.value("style", std::string("normal")) == "italic" ? ecs::CFontFace::Style::Italic
																  : ecs::CFontFace::Style::Normal;
	reg.emplace_or_replace<ecs::CFontFace>(e, std::move(f));
	return true;
}

bool serializeInstance(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CInstance>(e)) {
		return false;
	}
	const auto src = reg.get<ecs::CInstance>(e).source;
	if (src == entt::null) {
		return false;
	}
	j["source"] = entityIdString(src);
	return true;
}

bool deserializeInstance(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CInstance i;
	i.source = readEntityRef(j, "source");
	reg.emplace_or_replace<ecs::CInstance>(e, i);
	return true;
}

bool serializePaintClip(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPaintClip>(e)) {
		return false;
	}
	return serializeEntityRef(j, "path", reg.get<ecs::CPaintClip>(e).path);
}

bool deserializePaintClip(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPaintClip c;
	c.path = readEntityRef(j, "path");
	reg.emplace_or_replace<ecs::CPaintClip>(e, c);
	return true;
}

bool serializePaintMask(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPaintMask>(e)) {
		return false;
	}
	const auto& m = reg.get<ecs::CPaintMask>(e);
	if (m.source == entt::null) {
		return false;
	}
	j["source"] = entityIdString(m.source);
	if (m.channel == ecs::CPaintMask::Channel::Alpha) {
		j["channel"] = "alpha";
	}
	return true;
}

bool deserializePaintMask(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPaintMask m;
	m.source = readEntityRef(j, "source");
	m.channel = j.value("channel", std::string("luminance")) == "alpha"
					? ecs::CPaintMask::Channel::Alpha
					: ecs::CPaintMask::Channel::Luminance;
	reg.emplace_or_replace<ecs::CPaintMask>(e, m);
	return true;
}

bool serializePaintMarkers(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPaintMarkers>(e)) {
		return false;
	}
	const auto& m = reg.get<ecs::CPaintMarkers>(e);
	bool any = false;
	any = serializeEntityRef(j, "start", m.start) || any;
	any = serializeEntityRef(j, "mid", m.mid) || any;
	any = serializeEntityRef(j, "end", m.end) || any;
	return any;
}

bool deserializePaintMarkers(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPaintMarkers m;
	m.start = readEntityRef(j, "start");
	m.mid = readEntityRef(j, "mid");
	m.end = readEntityRef(j, "end");
	reg.emplace_or_replace<ecs::CPaintMarkers>(e, m);
	return true;
}

bool serializePaintMarker(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPaintMarker>(e)) {
		return false;
	}
	const auto& m = reg.get<ecs::CPaintMarker>(e);
	j["width"] = m.width;
	j["height"] = m.height;
	if (m.ref.x != 0.f || m.ref.y != 0.f) {
		j["ref"] = vec2ToJson(m.ref);
	}
	if (m.orient == ecs::CPaintMarker::Orient::AutoStartReverse) {
		j["orient"] = "auto-start-reverse";
	} else if (m.orient == ecs::CPaintMarker::Orient::Angle) {
		j["orient"] = "angle";
		j["orientDegrees"] = m.orientDegrees;
	}
	if (m.units == ecs::CPaintMarker::Units::UserSpace) {
		j["units"] = "user-space";
	}
	return true;
}

bool deserializePaintMarker(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPaintMarker m;
	m.width = j.value("width", m.width);
	m.height = j.value("height", m.height);
	if (j.contains("ref")) {
		m.ref = vec2FromJson(j["ref"], m.ref);
	}
	const std::string o = j.value("orient", std::string("auto"));
	m.orient = o == "auto-start-reverse" ? ecs::CPaintMarker::Orient::AutoStartReverse
			   : o == "angle"			 ? ecs::CPaintMarker::Orient::Angle
										 : ecs::CPaintMarker::Orient::Auto;
	m.orientDegrees = j.value("orientDegrees", m.orientDegrees);
	m.units = j.value("units", std::string("stroke-width")) == "user-space"
				  ? ecs::CPaintMarker::Units::UserSpace
				  : ecs::CPaintMarker::Units::StrokeWidth;
	reg.emplace_or_replace<ecs::CPaintMarker>(e, m);
	return true;
}

bool serializePaintPattern(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPaintPattern>(e)) {
		return false;
	}
	const auto& p = reg.get<ecs::CPaintPattern>(e);
	j["width"] = p.width;
	j["height"] = p.height;
	if (p.units == ecs::CPaintPattern::Units::UserSpace) {
		j["units"] = "user-space";
	}
	if (p.contentUnits == ecs::CPaintPattern::Units::Object) {
		j["contentUnits"] = "object";
	}
	return true;
}

bool deserializePaintPattern(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPaintPattern p;
	p.width = j.value("width", p.width);
	p.height = j.value("height", p.height);
	p.units = j.value("units", std::string("object")) == "user-space"
				  ? ecs::CPaintPattern::Units::UserSpace
				  : ecs::CPaintPattern::Units::Object;
	p.contentUnits = j.value("contentUnits", std::string("user-space")) == "object"
						 ? ecs::CPaintPattern::Units::Object
						 : ecs::CPaintPattern::Units::UserSpace;
	reg.emplace_or_replace<ecs::CPaintPattern>(e, p);
	return true;
}

bool serializePaintOrder(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPaintOrder>(e)) {
		return false;
	}
	const auto& o = reg.get<ecs::CPaintOrder>(e);
	if (o.layers.empty()) {
		return false;
	}
	auto layers = ordered_json::array();
	for (auto layer : o.layers) {
		layers.push_back(layer == ecs::CPaintOrder::Layer::Stroke	 ? "stroke"
						 : layer == ecs::CPaintOrder::Layer::Markers ? "markers"
																	 : "fill");
	}
	j["layers"] = std::move(layers);
	return true;
}

bool deserializePaintOrder(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPaintOrder o;
	if (j.contains("layers") && j["layers"].is_array()) {
		for (const auto& layer : j["layers"]) {
			const std::string s = layer.get<std::string>();
			o.layers.push_back(s == "stroke"	? ecs::CPaintOrder::Layer::Stroke
							   : s == "markers" ? ecs::CPaintOrder::Layer::Markers
												: ecs::CPaintOrder::Layer::Fill);
		}
	}
	reg.emplace_or_replace<ecs::CPaintOrder>(e, std::move(o));
	return true;
}

bool serializePaintFilter(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPaintFilter>(e)) {
		return false;
	}
	const auto& f = reg.get<ecs::CPaintFilter>(e);
	bool any = serializeEntityRef(j, "filter", f.filter);
	if (f.primitiveUnits == ecs::CPaintFilter::Units::Object) {
		j["primitiveUnits"] = "object";
		any = true;
	}
	return any;
}

bool deserializePaintFilter(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPaintFilter f;
	f.filter = readEntityRef(j, "filter");
	f.primitiveUnits = j.value("primitiveUnits", std::string("user-space")) == "object"
						   ? ecs::CPaintFilter::Units::Object
						   : ecs::CPaintFilter::Units::UserSpace;
	reg.emplace_or_replace<ecs::CPaintFilter>(e, f);
	return true;
}

const char* filterKindName(ecs::CPaintFilterStep::Kind k) {
	switch (k) {
	case ecs::CPaintFilterStep::Kind::Blend:
		return "blend";
	case ecs::CPaintFilterStep::Kind::ColorMatrix:
		return "color-matrix";
	case ecs::CPaintFilterStep::Kind::ComponentTransfer:
		return "component-transfer";
	case ecs::CPaintFilterStep::Kind::Composite:
		return "composite";
	case ecs::CPaintFilterStep::Kind::Convolve:
		return "convolve";
	case ecs::CPaintFilterStep::Kind::DiffuseLight:
		return "diffuse-light";
	case ecs::CPaintFilterStep::Kind::Displacement:
		return "displacement";
	case ecs::CPaintFilterStep::Kind::DropShadow:
		return "drop-shadow";
	case ecs::CPaintFilterStep::Kind::Flood:
		return "flood";
	case ecs::CPaintFilterStep::Kind::Blur:
		return "blur";
	case ecs::CPaintFilterStep::Kind::Image:
		return "image";
	case ecs::CPaintFilterStep::Kind::Merge:
		return "merge";
	case ecs::CPaintFilterStep::Kind::Morphology:
		return "morphology";
	case ecs::CPaintFilterStep::Kind::Offset:
		return "offset";
	case ecs::CPaintFilterStep::Kind::SpecularLight:
		return "specular-light";
	case ecs::CPaintFilterStep::Kind::Tile:
		return "tile";
	case ecs::CPaintFilterStep::Kind::Turbulence:
		return "turbulence";
	}
	return "blur";
}

ecs::CPaintFilterStep::Kind filterKindFromName(const std::string& s) {
	if (s == "blend") {
		return ecs::CPaintFilterStep::Kind::Blend;
	}
	if (s == "color-matrix") {
		return ecs::CPaintFilterStep::Kind::ColorMatrix;
	}
	if (s == "component-transfer") {
		return ecs::CPaintFilterStep::Kind::ComponentTransfer;
	}
	if (s == "composite") {
		return ecs::CPaintFilterStep::Kind::Composite;
	}
	if (s == "convolve") {
		return ecs::CPaintFilterStep::Kind::Convolve;
	}
	if (s == "diffuse-light") {
		return ecs::CPaintFilterStep::Kind::DiffuseLight;
	}
	if (s == "displacement") {
		return ecs::CPaintFilterStep::Kind::Displacement;
	}
	if (s == "drop-shadow") {
		return ecs::CPaintFilterStep::Kind::DropShadow;
	}
	if (s == "flood") {
		return ecs::CPaintFilterStep::Kind::Flood;
	}
	if (s == "image") {
		return ecs::CPaintFilterStep::Kind::Image;
	}
	if (s == "merge") {
		return ecs::CPaintFilterStep::Kind::Merge;
	}
	if (s == "morphology") {
		return ecs::CPaintFilterStep::Kind::Morphology;
	}
	if (s == "offset") {
		return ecs::CPaintFilterStep::Kind::Offset;
	}
	if (s == "specular-light") {
		return ecs::CPaintFilterStep::Kind::SpecularLight;
	}
	if (s == "tile") {
		return ecs::CPaintFilterStep::Kind::Tile;
	}
	if (s == "turbulence") {
		return ecs::CPaintFilterStep::Kind::Turbulence;
	}
	return ecs::CPaintFilterStep::Kind::Blur;
}

bool serializePaintFilterStep(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CPaintFilterStep>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CPaintFilterStep>(e);
	j["kind"] = filterKindName(s.kind);
	if (!s.in.empty()) {
		j["in"] = s.in;
	}
	if (!s.in2.empty()) {
		j["in2"] = s.in2;
	}
	if (s.stdDeviation != 0.f) {
		j["stdDeviation"] = s.stdDeviation;
	}
	if (s.offset.x != 0.f || s.offset.y != 0.f) {
		j["offset"] = vec2ToJson(s.offset);
	}
	if (!s.values.empty()) {
		j["values"] = s.values;
	}
	if (s.kind == ecs::CPaintFilterStep::Kind::Composite ||
		s.kind == ecs::CPaintFilterStep::Kind::Morphology) {
		const char* op = "over";
		switch (s.op) {
		case ecs::CPaintFilterStep::Operator::In:
			op = "in";
			break;
		case ecs::CPaintFilterStep::Operator::Out:
			op = "out";
			break;
		case ecs::CPaintFilterStep::Operator::Atop:
			op = "atop";
			break;
		case ecs::CPaintFilterStep::Operator::Xor:
			op = "xor";
			break;
		case ecs::CPaintFilterStep::Operator::Arithmetic:
			op = "arithmetic";
			break;
		case ecs::CPaintFilterStep::Operator::Erode:
			op = "erode";
			break;
		case ecs::CPaintFilterStep::Operator::Dilate:
			op = "dilate";
			break;
		default:
			break;
		}
		j["operator"] = op;
	}
	if (s.radius != 0.f) {
		j["radius"] = s.radius;
	}
	if (s.kind == ecs::CPaintFilterStep::Kind::Flood) {
		j["rgba"] = colorToJson(s.rgba);
	}
	serializeEntityRef(j, "asset", s.asset);
	return true;
}

bool deserializePaintFilterStep(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CPaintFilterStep s;
	s.kind = filterKindFromName(j.value("kind", std::string("blur")));
	s.in = j.value("in", s.in);
	s.in2 = j.value("in2", s.in2);
	s.stdDeviation = j.value("stdDeviation", s.stdDeviation);
	if (j.contains("offset")) {
		s.offset = vec2FromJson(j["offset"], s.offset);
	}
	if (j.contains("values") && j["values"].is_array()) {
		s.values = j["values"].get<std::vector<float>>();
	}
	const std::string op = j.value("operator", std::string("over"));
	s.op = op == "in"			? ecs::CPaintFilterStep::Operator::In
		   : op == "out"		? ecs::CPaintFilterStep::Operator::Out
		   : op == "atop"		? ecs::CPaintFilterStep::Operator::Atop
		   : op == "xor"		? ecs::CPaintFilterStep::Operator::Xor
		   : op == "arithmetic" ? ecs::CPaintFilterStep::Operator::Arithmetic
		   : op == "erode"		? ecs::CPaintFilterStep::Operator::Erode
		   : op == "dilate"		? ecs::CPaintFilterStep::Operator::Dilate
								: ecs::CPaintFilterStep::Operator::Over;
	s.radius = j.value("radius", s.radius);
	if (j.contains("rgba")) {
		s.rgba = rgbaFromJson(j["rgba"], s.rgba);
	}
	s.asset = readEntityRef(j, "asset");
	reg.emplace_or_replace<ecs::CPaintFilterStep>(e, std::move(s));
	return true;
}

bool serializeCurve(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CCurve>(e)) {
		return false;
	}
	const auto& c = reg.get<ecs::CCurve>(e);
	auto points = ordered_json::array();
	for (const auto& p : c.points) {
		points.push_back(vec2ToJson(p));
	}
	j["points"] = std::move(points);
	j["interpolation"] = curve::interpId(c.interp);
	if (c.interp == ecs::CCurve::Interp::Bezier) {
		auto handles = ordered_json::array();
		for (const auto& h : c.handles) {
			handles.push_back({{"in", vec2ToJson(h.in)}, {"out", vec2ToJson(h.out)}});
		}
		j["handles"] = std::move(handles);
	}
	if (c.preset != ecs::CCurve::Preset::Custom || c.points.size() >= 2) {
		j["preset"] = curve::presetId(c.preset);
	}
	return true;
}

bool deserializeCurve(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CCurve c;
	if (j.contains("points") && j["points"].is_array()) {
		c.points.clear();
		for (const auto& p : j["points"]) {
			c.points.push_back(vec2FromJson(p));
		}
	}
	if (j.contains("interpolation") && j["interpolation"].is_string()) {
		c.interp = curve::interpFromId(j["interpolation"].get<std::string>());
	} else if (j.contains("interp") && j["interp"].is_string()) {
		c.interp = curve::interpFromId(j["interp"].get<std::string>());
	}
	if (j.contains("preset") && j["preset"].is_string()) {
		c.preset = curve::presetFromId(j["preset"].get<std::string>());
	}
	if (j.contains("handles") && j["handles"].is_array()) {
		c.handles.clear();
		for (const auto& h : j["handles"]) {
			ecs::CCurve::Handle hd;
			if (h.contains("in")) {
				hd.in = vec2FromJson(h["in"]);
			}
			if (h.contains("out")) {
				hd.out = vec2FromJson(h["out"]);
			}
			c.handles.push_back(hd);
		}
	}
	curve::ensureValid(c);
	reg.emplace_or_replace<ecs::CCurve>(e, std::move(c));
	return true;
}

void registerInto(project::ComponentSerializerRegistry& registry) {
	using project::addBoolMemberSerializer;
	using project::addMarkerSerializer;
	using project::addSerializer;
	using project::EntityIdMap;

	// Contract schemas where one exists; x.rigkit.* where the concept is ours.
	addSerializer<ecs::CTransform>(registry, "Transform", "rig.spatial.transform",
								   serializeTransform, deserializeTransform);
	addSerializer<ecs::CRelationship>(registry, "Relationship", "rig.spatial.relationship",
									  serializeRelationship, deserializeRelationship);
	addMarkerSerializer<ecs::CGroup>(registry, "Group", "rig.spatial.group");
	addSerializer<ecs::CCamera>(registry, "Camera", "rig.spatial.camera", serializeCamera,
								deserializeCamera);
	addBoolMemberSerializer<ecs::CSelectable, &ecs::CSelectable::enabled>(
		registry, "Selectable", "rig.interact.selectable", "enabled");
	addSerializer<ecs::CDrawStyle>(registry, "DrawStyle", "rig.paint.fill_stroke",
								   serializeDrawStyle, deserializeDrawStyle);
	addSerializer<ecs::CLight>(registry, "Light", "rig.render.light", serializeLight,
							   deserializeLight);
	addSerializer<ecs::CRenderEnvironment>(
		registry, "RenderEnvironment", "rig.render.environment", serializeEnvironment,
		deserializeEnvironment, remapEnvironment);
	addSerializer<ecs::CDrawStyle>(registry, "StrokeStyle", "rig.paint.stroke_style",
								   serializeStrokeStyle, deserializeStrokeStyle);
	addSerializer<ecs::CDrawStyle>(registry, "StrokeStyleLegacy", "x.rigkit.stroke_style",
								   serializeStrokeStyleLegacy, deserializeStrokeStyle);
	addSerializer<ecs::CLight>(registry, "LightShading", "x.rigkit.light_shading",
							   serializeLightShading, deserializeLightShading);

	addSerializer<ecs::CRectangle>(registry, "Rectangle", "rig.geometry.rectangle",
								   serializeRectangle, deserializeRectangle);
	addSerializer<ecs::CEllipse>(registry, "Ellipse", "rig.geometry.ellipse", serializeEllipse,
								 deserializeEllipse);
	addSerializer<ecs::CLine>(registry, "Line", "rig.geometry.line", serializeLine,
							  deserializeLine);
	addSerializer<ecs::CPolygon>(registry, "Polygon", "rig.geometry.polygon", serializePolygon,
								 deserializePolygon);
	addSerializer<ecs::CRegularPolygon>(registry, "RegularPolygon", "rig.geometry.regular_polygon",
										serializeRegularPolygon, deserializeRegularPolygon);
	addSerializer<ecs::CStar>(registry, "Star", "rig.geometry.star", serializeStar,
							  deserializeStar);
	addSerializer<ecs::CArc>(registry, "Arc", "rig.geometry.arc", serializeArc, deserializeArc);
	addSerializer<ecs::CSpline>(registry, "Spline", "rig.geometry.spline", serializeSpline,
								deserializeSpline);
	addSerializer<ecs::CSpline3d>(registry, "Spline3d", "rig.geometry.spline3d", serializeSpline3d,
								  deserializeSpline3d);
	addSerializer<ecs::CNurbsSurface>(registry, "NurbsSurface", "rig.geometry.nurbs_surface",
									  serializeNurbsSurface, deserializeNurbsSurface);
	addSerializer<ecs::CRing>(registry, "Ring", "rig.geometry.ring", serializeRing,
							  deserializeRing);
	addSerializer<ecs::CMesh>(registry, "Mesh", "rig.geometry.mesh", serializeMesh,
							  deserializeMesh);
	addSerializer<ecs::CPath>(registry, "Path", "rig.geometry.path", serializePath,
							  deserializePath);

	addSerializer<ecs::CLayer>(registry, "Layer", "rig.spatial.layer", serializeLayer,
							   deserializeLayer);
	addSerializer<ecs::CLayer>(registry, "LayerVisible", "x.rigkit.layer_visible",
							   serializeLayerVisible, deserializeLayerVisible);
	addSerializer<ecs::CRenderVisibility>(registry, "RenderVisibility", "rig.render.visibility",
										  serializeRenderVisibility, deserializeRenderVisibility);
	addSerializer<ecs::CRenderBlend>(registry, "RenderBlend", "rig.render.blend",
									 serializeRenderBlend, deserializeRenderBlend);
	addSerializer<ecs::CUiPanel>(registry, "UiPanel", "rig.ui.panel", serializeUiPanel,
								 deserializeUiPanel);
	addSerializer<ecs::CUiGroup>(registry, "UiGroup", "rig.ui.group", serializeUiGroup,
								 deserializeUiGroup, remapUiRefs);
	addSerializer<ecs::CUiControl>(registry, "UiControl", "rig.ui.control", serializeUiControl,
								   deserializeUiControl);
	addSerializer<ecs::CAssetRef>(registry, "AssetRef", "rig.media.asset_ref", serializeAssetRef,
								  deserializeAssetRef);
	addSerializer<ecs::CText>(
		registry, "Text", "rig.media.text", serializeText, deserializeText,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapTextFont(reg, idMap); });
	addSerializer<ecs::CTextPath>(
		registry, "TextPath", "rig.media.text_path", serializeTextPath, deserializeTextPath,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapTextPath(reg, idMap); });
	addSerializer<ecs::CFontFace>(
		registry, "FontFace", "rig.media.font_face", serializeFontFace, deserializeFontFace,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapFontFace(reg, idMap); });
	addSerializer<ecs::CInstance>(
		registry, "Instance", "rig.spatial.instance", serializeInstance, deserializeInstance,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapInstance(reg, idMap); });
	addSerializer<ecs::CPaintClip>(
		registry, "PaintClip", "rig.paint.clip", serializePaintClip, deserializePaintClip,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapPaintClip(reg, idMap); });
	addSerializer<ecs::CPaintMask>(
		registry, "PaintMask", "rig.paint.mask", serializePaintMask, deserializePaintMask,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapPaintMask(reg, idMap); });
	addSerializer<ecs::CPaintMarkers>(
		registry, "PaintMarkers", "rig.paint.markers", serializePaintMarkers, deserializePaintMarkers,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapPaintMarkers(reg, idMap); });
	addSerializer<ecs::CPaintMarker>(registry, "PaintMarker", "rig.paint.marker",
									 serializePaintMarker, deserializePaintMarker);
	addSerializer<ecs::CPaintPattern>(registry, "PaintPattern", "rig.paint.pattern",
									  serializePaintPattern, deserializePaintPattern);
	addSerializer<ecs::CPaintOrder>(registry, "PaintOrder", "rig.paint.order", serializePaintOrder,
									deserializePaintOrder);
	addSerializer<ecs::CPaintFilter>(
		registry, "PaintFilter", "rig.paint.filter", serializePaintFilter, deserializePaintFilter,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapPaintFilter(reg, idMap); });
	addSerializer<ecs::CPaintFilterStep>(
		registry, "PaintFilterStep", "rig.paint.filter_step", serializePaintFilterStep,
		deserializePaintFilterStep,
		[](entt::registry& reg, const EntityIdMap& idMap) { remapPaintFilterStep(reg, idMap); });
	addSerializer<ecs::CCurve>(registry, "Curve", "rig.anim.curve", serializeCurve,
							   deserializeCurve);

	addSerializer<ecs::CPalette>(registry, "Palette", "rig.pixel.palette", serializePalette,
								 deserializePalette);
	addSerializer<ecs::CPalette>(registry, "PaletteShade", "x.rigkit.palette_shade",
								 serializePaletteShade, deserializePaletteShade);
	// No Contract schema covers these yet, so they travel as host extensions.
	addSerializer<ecs::CGuide>(registry, "Guide", "x.rigkit.guide", serializeGuide,
							   deserializeGuide);
	addSerializer<ecs::CIndexedAtlas>(registry, "IndexedAtlas", "x.rigkit.indexed_atlas",
									  serializeIndexedAtlas, deserializeIndexedAtlas);
	addSerializer<ecs::CFaceSelection>(registry, "FaceSelection", "x.rigkit.face_selection",
									   serializeFaceSelection, deserializeFaceSelection);
	addSerializer<ecs::CEdgeSelection>(registry, "EdgeSelection", "x.rigkit.edge_selection",
									   serializeEdgeSelection, deserializeEdgeSelection);
}

} // namespace

namespace project {

void registerComponentSerializers(ComponentSerializerRegistry& registry) {
	registerInto(registry);
}

} // namespace project

} // namespace rigkit
