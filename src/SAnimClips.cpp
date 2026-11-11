#include "SAnimClips.h"

#include "CAnimClip.h"
#include "CAnimTrack.h"
#include "CTransform.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace rigkit {
namespace ecs {
namespace {

float wrapTime(float t, float duration, const std::string& wrap) {
	if (duration <= 1e-6f) {
		return 0.f;
	}
	if (wrap == "loop") {
		t = std::fmod(t, duration);
		if (t < 0.f) {
			t += duration;
		}
		return t;
	}
	if (wrap == "ping-pong") {
		const float cycle = duration * 2.f;
		t = std::fmod(t, cycle);
		if (t < 0.f) {
			t += cycle;
		}
		return (t <= duration) ? t : (cycle - t);
	}
	return std::clamp(t, 0.f, duration);
}

void sampleAt(const CAnimTrack& track, float t, float* out) {
	if (track.times.empty() || track.stride < 1) {
		return;
	}
	const int stride = track.stride;
	if (track.times.size() == 1) {
		for (int i = 0; i < stride; ++i) {
			out[i] = track.values[static_cast<size_t>(i)];
		}
		return;
	}
	size_t hi = 0;
	while (hi + 1 < track.times.size() && track.times[hi + 1] < t) {
		++hi;
	}
	if (hi + 1 >= track.times.size() || t <= track.times[hi]) {
		const size_t base = hi * static_cast<size_t>(stride);
		for (int i = 0; i < stride; ++i) {
			out[i] = track.values[base + static_cast<size_t>(i)];
		}
		return;
	}
	const size_t lo = hi;
	hi = lo + 1;
	const float t0 = track.times[lo];
	const float t1 = track.times[hi];
	const float u = (t1 > t0) ? ((t - t0) / (t1 - t0)) : 0.f;
	const size_t b0 = lo * static_cast<size_t>(stride);
	const size_t b1 = hi * static_cast<size_t>(stride);
	if (stride == 4) {
		const glm::quat a(track.values[b0 + 3], track.values[b0 + 0], track.values[b0 + 1],
						  track.values[b0 + 2]);
		const glm::quat b(track.values[b1 + 3], track.values[b1 + 0], track.values[b1 + 1],
						  track.values[b1 + 2]);
		const glm::quat q = glm::normalize(glm::slerp(a, b, u));
		out[0] = q.x;
		out[1] = q.y;
		out[2] = q.z;
		out[3] = q.w;
		return;
	}
	for (int i = 0; i < stride; ++i) {
		const float a = track.values[b0 + static_cast<size_t>(i)];
		const float b = track.values[b1 + static_cast<size_t>(i)];
		out[i] = a + (b - a) * u;
	}
}

} // namespace

void SAnimClips(MEcs& ecs, float dt) {
	for (auto entity : ecs.view<CAnimClip>()) {
		auto& clip = ecs.getComponent<CAnimClip>(entity);
		if (!clip.playing || clip.duration <= 0.f) {
			continue;
		}
		clip.elapsed += dt;
	}

	for (auto entity : ecs.view<CAnimTrack>()) {
		const auto& track = ecs.getComponent<CAnimTrack>(entity);
		if (track.clip == entt::null || track.target == entt::null ||
			!ecs.registry().valid(track.clip) || !ecs.registry().valid(track.target) ||
			!ecs.hasComponent<CAnimClip>(track.clip) ||
			!ecs.hasComponent<CTransform>(track.target)) {
			continue;
		}
		const auto& clip = ecs.getComponent<CAnimClip>(track.clip);
		if (!clip.playing) {
			continue;
		}
		const float t = wrapTime(clip.elapsed, clip.duration, clip.wrap);
		float sample[4] = {0.f, 0.f, 0.f, 1.f};
		sampleAt(track, t, sample);
		auto& xf = ecs.getComponent<CTransform>(track.target);
		if (track.propertyKey == "rotation" && track.stride == 4) {
			xf.setRotationQuat(glm::quat(sample[3], sample[0], sample[1], sample[2]));
		} else if (track.propertyKey == "position" && track.stride == 3) {
			xf.position = {sample[0], sample[1], sample[2]};
		} else if (track.propertyKey == "scale" && track.stride == 3) {
			xf.scale = {sample[0], sample[1], sample[2]};
		}
	}
}

} // namespace ecs
} // namespace rigkit
