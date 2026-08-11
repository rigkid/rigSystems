#include "SModulators.h"

#include "CModBinding.h"
#include "CModLfo.h"
#include "EntityProperty.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace rigkit {
namespace ecs {
namespace {

constexpr float kTwoPi = 6.283185307179586f;

} // namespace

void SModulators(MEcs& ecs, float dt) {
	for (auto e : ecs.view<ecs::CModLfo>()) {
		auto& lfo = ecs.getComponent<ecs::CModLfo>(e);
		const float freq = std::max(0.f, lfo.frequency);
		lfo.phase = lfo.phase + dt * freq;
		lfo.phase = lfo.phase - std::floor(lfo.phase);
		float w = 0.f;
		if (lfo.waveform == "tri") {
			w = 1.f - 4.f * std::fabs(lfo.phase - 0.5f);
		} else if (lfo.waveform == "saw") {
			w = lfo.phase * 2.f - 1.f;
		} else if (lfo.waveform == "square") {
			w = lfo.phase < 0.5f ? 1.f : -1.f;
		} else {
			w = std::sin(lfo.phase * kTwoPi);
		}
		lfo.lastSample = lfo.offset + lfo.amplitude * w;
	}

	for (auto e : ecs.view<ecs::CModBinding>()) {
		const auto& b = ecs.getComponent<ecs::CModBinding>(e);
		if (b.source.empty() || b.target.empty() || b.propertyKey.empty()) {
			continue;
		}
		const entt::entity src = ecs.findEntity(b.source);
		if (src == entt::null || !ecs.hasComponent<ecs::CModLfo>(src)) {
			continue;
		}
		float v = ecs.getComponent<ecs::CModLfo>(src).lastSample * b.depth;
		if (b.hasMin) {
			v = std::max(b.min, v);
		}
		if (b.hasMax) {
			v = std::min(b.max, v);
		}
		if (b.additive) {
			if (auto cur = readEntityProperty(ecs, b.target, b.propertyKey)) {
				v = *cur + v;
			}
		}
		writeEntityProperty(ecs, b.target, b.propertyKey, v);
	}
}

} // namespace ecs
} // namespace rigkit
