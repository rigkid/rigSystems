#include "STweens.h"

#include "CTween.h"
#include "EntityProperty.h"

#include <algorithm>
#include <cmath>

namespace rigkit {
namespace ecs {
namespace {

float ease(float t, const std::string& name) {
	t = std::clamp(t, 0.f, 1.f);
	if (name == "ease-in") {
		return t * t;
	}
	if (name == "ease-out") {
		return 1.f - (1.f - t) * (1.f - t);
	}
	if (name == "ease-in-out") {
		return t < 0.5f ? 2.f * t * t : 1.f - std::pow(-2.f * t + 2.f, 2.f) * 0.5f;
	}
	return t;
}

} // namespace

void STweens(MEcs& ecs, float dt) {
	for (auto e : ecs.view<ecs::CTween>()) {
		auto& tw = ecs.getComponent<ecs::CTween>(e);
		if (!tw.playing || tw.target.empty() || tw.propertyKey.empty()) {
			continue;
		}
		const float dur = std::max(1e-6f, tw.duration);
		tw.elapsed += dt;
		if (tw.loop) {
			tw.elapsed = tw.elapsed - dur * std::floor(tw.elapsed / dur);
		} else if (tw.elapsed >= dur) {
			tw.elapsed = dur;
			tw.playing = false;
		}
		const float u = ease(tw.elapsed / dur, tw.easing);
		const float v = tw.from + (tw.to - tw.from) * u;
		writeEntityProperty(ecs, tw.target, tw.propertyKey, v);
	}
}

} // namespace ecs
} // namespace rigkit
