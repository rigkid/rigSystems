#include "AnimSerializers.h"

#include "AddSerializer.h"
#include "CModBinding.h"
#include "CModLfo.h"
#include "CTween.h"

namespace rigkit {
namespace {
using project::ordered_json;

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

} // namespace

namespace anim {

void registerProjectSerializers(project::ComponentSerializerRegistry& registry) {
	project::addSerializer<ecs::CModLfo>(registry, "ModLfo", "rig.mod.lfo", serializeModLfo,
										 deserializeModLfo);
	project::addSerializer<ecs::CModBinding>(registry, "ModBinding", "rig.mod.binding",
											 serializeModBinding, deserializeModBinding);
	project::addSerializer<ecs::CTween>(registry, "Tween", "rig.anim.tween", serializeTween,
										deserializeTween);
}

} // namespace anim
} // namespace rigkit
