#include "AnimSerializers.h"

#include "AddSerializer.h"
#include "CAnimBind.h"
#include "CAnimClip.h"
#include "CAnimSkeleton.h"
#include "CAnimSkin.h"
#include "CAnimTrack.h"
#include "CModBinding.h"
#include "CModLfo.h"
#include "CTween.h"
#include "EntityIdRemap.h"
#include "core/TypeJson.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace rigkit {
namespace {
using project::entityIdFromString;
using project::EntityIdMap;
using project::entityIdString;
using project::ordered_json;

void remapEntityField(entt::entity& e, const EntityIdMap& idMap) {
	if (e == entt::null) {
		return;
	}
	const auto it = idMap.find(static_cast<std::uint32_t>(e));
	e = (it != idMap.end()) ? it->second : entt::null;
}

std::unordered_map<entt::entity, std::string> gPendingSkeletonRoot;
std::unordered_map<entt::entity, std::vector<std::string>> gPendingSkeletonBones;
std::unordered_map<entt::entity, std::string> gPendingSkinSkeleton;
std::unordered_map<entt::entity, std::vector<std::string>> gPendingSkinJoints;
std::unordered_map<entt::entity, std::string> gPendingTrackClip;
std::unordered_map<entt::entity, std::string> gPendingTrackTarget;

bool serializeModLfo(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CModLfo>(e)) {
		return false;
	}
	const auto& m = reg.get<ecs::CModLfo>(e);
	j["waveform"] = m.waveform;
	j["frequency"] = m.frequency;
	j["amplitude"] = m.amplitude;
	j["offset"] = m.offset;
	j["phase"] = m.phase;
	return true;
}

bool deserializeModLfo(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CModLfo m;
	m.waveform = j.value("waveform", m.waveform);
	m.frequency = j.value("frequency", m.frequency);
	m.amplitude = j.value("amplitude", m.amplitude);
	m.offset = j.value("offset", m.offset);
	m.phase = j.value("phase", m.phase);
	reg.emplace_or_replace<ecs::CModLfo>(e, m);
	return true;
}

bool serializeModBinding(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CModBinding>(e)) {
		return false;
	}
	const auto& b = reg.get<ecs::CModBinding>(e);
	j["source"] = b.source;
	j["target"] = b.target;
	j["propertyKey"] = b.propertyKey;
	j["depth"] = b.depth;
	j["additive"] = b.additive;
	if (b.hasMin) {
		j["min"] = b.min;
	}
	if (b.hasMax) {
		j["max"] = b.max;
	}
	return true;
}

bool deserializeModBinding(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CModBinding b;
	b.source = j.value("source", b.source);
	b.target = j.value("target", b.target);
	b.propertyKey = j.value("propertyKey", b.propertyKey);
	b.depth = j.value("depth", b.depth);
	b.additive = j.value("additive", b.additive);
	if (j.contains("min") && j["min"].is_number()) {
		b.min = j["min"].get<float>();
		b.hasMin = true;
	}
	if (j.contains("max") && j["max"].is_number()) {
		b.max = j["max"].get<float>();
		b.hasMax = true;
	}
	reg.emplace_or_replace<ecs::CModBinding>(e, b);
	return true;
}

bool serializeTween(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CTween>(e)) {
		return false;
	}
	const auto& t = reg.get<ecs::CTween>(e);
	j["target"] = t.target;
	j["propertyKey"] = t.propertyKey;
	j["from"] = t.from;
	j["to"] = t.to;
	j["duration"] = t.duration;
	j["elapsed"] = t.elapsed;
	j["easing"] = t.easing;
	j["loop"] = t.loop;
	j["playing"] = t.playing;
	return true;
}

bool deserializeTween(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CTween t;
	t.target = j.value("target", t.target);
	t.propertyKey = j.value("propertyKey", t.propertyKey);
	t.from = j.value("from", t.from);
	t.to = j.value("to", t.to);
	t.duration = j.value("duration", t.duration);
	t.elapsed = j.value("elapsed", t.elapsed);
	t.easing = j.value("easing", t.easing);
	t.loop = j.value("loop", t.loop);
	t.playing = j.value("playing", t.playing);
	reg.emplace_or_replace<ecs::CTween>(e, t);
	return true;
}

void remapName(std::string& value,
			   const std::unordered_map<std::string, std::string>& nameByDocumentId) {
	const auto it = nameByDocumentId.find(value);
	if (it != nameByDocumentId.end()) {
		value = it->second;
	}
}

void remapBindingContractRefs(entt::registry& reg,
							  const std::unordered_map<std::string, std::string>& nameByDocumentId) {
	for (auto e : reg.view<ecs::CModBinding>()) {
		auto& b = reg.get<ecs::CModBinding>(e);
		remapName(b.source, nameByDocumentId);
		remapName(b.target, nameByDocumentId);
	}
}

void remapTweenContractRefs(entt::registry& reg,
							const std::unordered_map<std::string, std::string>& nameByDocumentId) {
	for (auto e : reg.view<ecs::CTween>()) {
		remapName(reg.get<ecs::CTween>(e).target, nameByDocumentId);
	}
}

bool serializeSkeleton(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CAnimSkeleton>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CAnimSkeleton>(e);
	if (s.root == entt::null) {
		return false;
	}
	j["root"] = entityIdString(s.root);
	ordered_json bones = ordered_json::array();
	for (auto bone : s.bones) {
		bones.push_back(entityIdString(bone));
	}
	j["bones"] = std::move(bones);
	return true;
}

bool deserializeSkeleton(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CAnimSkeleton s;
	if (j.contains("root") && j["root"].is_string()) {
		const auto id = j["root"].get<std::string>();
		s.root = entityIdFromString(id);
		if (s.root == entt::null && !id.empty()) {
			gPendingSkeletonRoot[e] = id;
		}
	}
	if (j.contains("bones") && j["bones"].is_array()) {
		std::vector<std::string> pending;
		for (const auto& bone : j["bones"]) {
			if (!bone.is_string()) {
				continue;
			}
			const auto id = bone.get<std::string>();
			const auto handle = entityIdFromString(id);
			if (handle != entt::null) {
				s.bones.push_back(handle);
			} else if (!id.empty()) {
				pending.push_back(id);
			}
		}
		if (!pending.empty()) {
			gPendingSkeletonBones[e] = std::move(pending);
		}
	}
	reg.emplace_or_replace<ecs::CAnimSkeleton>(e, s);
	return true;
}

void remapSkeleton(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto e : reg.view<ecs::CAnimSkeleton>()) {
		auto& s = reg.get<ecs::CAnimSkeleton>(e);
		remapEntityField(s.root, idMap);
		for (auto& bone : s.bones) {
			remapEntityField(bone, idMap);
		}
	}
}

void resolveSkeleton(entt::registry& reg,
					 const std::unordered_map<std::string, entt::entity>& byDoc) {
	for (auto it = gPendingSkeletonRoot.begin(); it != gPendingSkeletonRoot.end();) {
		if (reg.valid(it->first) && reg.all_of<ecs::CAnimSkeleton>(it->first)) {
			const auto found = byDoc.find(it->second);
			if (found != byDoc.end()) {
				reg.get<ecs::CAnimSkeleton>(it->first).root = found->second;
			}
		}
		it = gPendingSkeletonRoot.erase(it);
	}
	for (auto it = gPendingSkeletonBones.begin(); it != gPendingSkeletonBones.end();) {
		if (reg.valid(it->first) && reg.all_of<ecs::CAnimSkeleton>(it->first)) {
			auto& bones = reg.get<ecs::CAnimSkeleton>(it->first).bones;
			bones.clear();
			for (const auto& id : it->second) {
				const auto found = byDoc.find(id);
				if (found != byDoc.end()) {
					bones.push_back(found->second);
				}
			}
		}
		it = gPendingSkeletonBones.erase(it);
	}
}

bool serializeBind(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CAnimBind>(e)) {
		return false;
	}
	const auto& b = reg.get<ecs::CAnimBind>(e);
	if (b.hasPosition) {
		j["position"] = vec3ToJson(b.position);
	}
	if (b.hasRotation) {
		j["rotation"] = quatToJson(b.rotation);
	}
	if (b.hasScale) {
		j["scale"] = vec3ToJson(b.scale);
	}
	return b.hasPosition || b.hasRotation || b.hasScale;
}

bool deserializeBind(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CAnimBind b;
	if (j.contains("position")) {
		b.position = vec3FromJson(j["position"], b.position);
		b.hasPosition = true;
	}
	if (j.contains("rotation")) {
		b.rotation = quatFromJson(j["rotation"], b.rotation);
		b.hasRotation = true;
	}
	if (j.contains("scale")) {
		b.scale = vec3FromJson(j["scale"], b.scale);
		b.hasScale = true;
	}
	reg.emplace_or_replace<ecs::CAnimBind>(e, b);
	return true;
}

bool serializeSkin(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CAnimSkin>(e)) {
		return false;
	}
	const auto& s = reg.get<ecs::CAnimSkin>(e);
	if (s.skeleton == entt::null || s.joints.empty()) {
		return false;
	}
	j["skeleton"] = entityIdString(s.skeleton);
	ordered_json joints = ordered_json::array();
	for (auto joint : s.joints) {
		joints.push_back(entityIdString(joint));
	}
	j["joints"] = std::move(joints);
	j["inverseBind"] = s.inverseBind;
	j["jointIndices"] = s.jointIndices;
	j["jointWeights"] = s.jointWeights;
	return true;
}

bool deserializeSkin(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CAnimSkin s;
	if (j.contains("skeleton") && j["skeleton"].is_string()) {
		const auto id = j["skeleton"].get<std::string>();
		s.skeleton = entityIdFromString(id);
		if (s.skeleton == entt::null && !id.empty()) {
			gPendingSkinSkeleton[e] = id;
		}
	}
	if (j.contains("joints") && j["joints"].is_array()) {
		std::vector<std::string> pending;
		for (const auto& joint : j["joints"]) {
			if (!joint.is_string()) {
				continue;
			}
			const auto id = joint.get<std::string>();
			const auto handle = entityIdFromString(id);
			if (handle != entt::null) {
				s.joints.push_back(handle);
			} else if (!id.empty()) {
				pending.push_back(id);
			}
		}
		if (!pending.empty()) {
			gPendingSkinJoints[e] = std::move(pending);
		}
	}
	if (j.contains("inverseBind") && j["inverseBind"].is_array()) {
		s.inverseBind = j["inverseBind"].get<std::vector<float>>();
	}
	if (j.contains("jointIndices") && j["jointIndices"].is_array()) {
		s.jointIndices = j["jointIndices"].get<std::vector<uint32_t>>();
	}
	if (j.contains("jointWeights") && j["jointWeights"].is_array()) {
		s.jointWeights = j["jointWeights"].get<std::vector<float>>();
	}
	reg.emplace_or_replace<ecs::CAnimSkin>(e, s);
	return true;
}

void remapSkin(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto e : reg.view<ecs::CAnimSkin>()) {
		auto& s = reg.get<ecs::CAnimSkin>(e);
		remapEntityField(s.skeleton, idMap);
		for (auto& joint : s.joints) {
			remapEntityField(joint, idMap);
		}
	}
}

void resolveSkin(entt::registry& reg, const std::unordered_map<std::string, entt::entity>& byDoc) {
	for (auto it = gPendingSkinSkeleton.begin(); it != gPendingSkinSkeleton.end();) {
		if (reg.valid(it->first) && reg.all_of<ecs::CAnimSkin>(it->first)) {
			const auto found = byDoc.find(it->second);
			if (found != byDoc.end()) {
				reg.get<ecs::CAnimSkin>(it->first).skeleton = found->second;
			}
		}
		it = gPendingSkinSkeleton.erase(it);
	}
	for (auto it = gPendingSkinJoints.begin(); it != gPendingSkinJoints.end();) {
		if (reg.valid(it->first) && reg.all_of<ecs::CAnimSkin>(it->first)) {
			auto& joints = reg.get<ecs::CAnimSkin>(it->first).joints;
			joints.clear();
			for (const auto& id : it->second) {
				const auto found = byDoc.find(id);
				if (found != byDoc.end()) {
					joints.push_back(found->second);
				}
			}
		}
		it = gPendingSkinJoints.erase(it);
	}
}

bool serializeClip(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CAnimClip>(e)) {
		return false;
	}
	const auto& c = reg.get<ecs::CAnimClip>(e);
	j["duration"] = c.duration;
	if (c.wrap != "once") {
		j["wrap"] = c.wrap;
	}
	return true;
}

bool deserializeClip(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CAnimClip c;
	c.duration = j.value("duration", c.duration);
	c.wrap = j.value("wrap", c.wrap);
	c.playing = true;
	c.elapsed = 0.f;
	reg.emplace_or_replace<ecs::CAnimClip>(e, c);
	return true;
}

bool serializeTrack(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CAnimTrack>(e)) {
		return false;
	}
	const auto& t = reg.get<ecs::CAnimTrack>(e);
	if (t.clip == entt::null || t.target == entt::null) {
		return false;
	}
	j["clip"] = entityIdString(t.clip);
	j["target"] = entityIdString(t.target);
	j["propertyKey"] = t.propertyKey;
	j["times"] = t.times;
	j["values"] = t.values;
	j["stride"] = t.stride;
	return true;
}

bool deserializeTrack(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CAnimTrack t;
	if (j.contains("clip") && j["clip"].is_string()) {
		const auto id = j["clip"].get<std::string>();
		t.clip = entityIdFromString(id);
		if (t.clip == entt::null && !id.empty()) {
			gPendingTrackClip[e] = id;
		}
	}
	if (j.contains("target") && j["target"].is_string()) {
		const auto id = j["target"].get<std::string>();
		t.target = entityIdFromString(id);
		if (t.target == entt::null && !id.empty()) {
			gPendingTrackTarget[e] = id;
		}
	}
	t.propertyKey = j.value("propertyKey", t.propertyKey);
	if (j.contains("times") && j["times"].is_array()) {
		t.times = j["times"].get<std::vector<float>>();
	}
	if (j.contains("values") && j["values"].is_array()) {
		t.values = j["values"].get<std::vector<float>>();
	}
	t.stride = j.value("stride", t.stride);
	reg.emplace_or_replace<ecs::CAnimTrack>(e, t);
	return true;
}

void remapTrack(entt::registry& reg, const EntityIdMap& idMap) {
	for (auto e : reg.view<ecs::CAnimTrack>()) {
		auto& t = reg.get<ecs::CAnimTrack>(e);
		remapEntityField(t.clip, idMap);
		remapEntityField(t.target, idMap);
	}
}

void resolveTrack(entt::registry& reg, const std::unordered_map<std::string, entt::entity>& byDoc) {
	for (auto it = gPendingTrackClip.begin(); it != gPendingTrackClip.end();) {
		if (reg.valid(it->first) && reg.all_of<ecs::CAnimTrack>(it->first)) {
			const auto found = byDoc.find(it->second);
			if (found != byDoc.end()) {
				reg.get<ecs::CAnimTrack>(it->first).clip = found->second;
			}
		}
		it = gPendingTrackClip.erase(it);
	}
	for (auto it = gPendingTrackTarget.begin(); it != gPendingTrackTarget.end();) {
		if (reg.valid(it->first) && reg.all_of<ecs::CAnimTrack>(it->first)) {
			const auto found = byDoc.find(it->second);
			if (found != byDoc.end()) {
				reg.get<ecs::CAnimTrack>(it->first).target = found->second;
			}
		}
		it = gPendingTrackTarget.erase(it);
	}
}

} // namespace

namespace anim {

void registerProjectSerializers(project::ComponentSerializerRegistry& registry) {
	project::addSerializer<ecs::CModLfo>(registry, "ModLfo", "rig.mod.lfo", serializeModLfo,
										 deserializeModLfo);
	project::addSerializer<ecs::CModBinding>(registry, "ModBinding", "rig.mod.binding",
											 serializeModBinding, deserializeModBinding, {},
											 remapBindingContractRefs);
	project::addSerializer<ecs::CTween>(registry, "Tween", "rig.anim.tween", serializeTween,
										deserializeTween, {}, remapTweenContractRefs);
	project::addSerializer<ecs::CAnimSkeleton>(registry, "AnimSkeleton", "rig.anim.skeleton",
											   serializeSkeleton, deserializeSkeleton, remapSkeleton,
											   {}, resolveSkeleton);
	project::addSerializer<ecs::CAnimBind>(registry, "AnimBind", "rig.anim.bind", serializeBind,
										   deserializeBind);
	project::addSerializer<ecs::CAnimSkin>(registry, "AnimSkin", "rig.anim.skin", serializeSkin,
										   deserializeSkin, remapSkin, {}, resolveSkin);
	project::addSerializer<ecs::CAnimClip>(registry, "AnimClip", "rig.anim.clip", serializeClip,
										   deserializeClip);
	project::addSerializer<ecs::CAnimTrack>(registry, "AnimTrack", "rig.anim.track", serializeTrack,
											deserializeTrack, remapTrack, {}, resolveTrack);
}

} // namespace anim
} // namespace rigkit
