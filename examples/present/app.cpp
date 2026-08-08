#include "app.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigComponent/src/CTransform.h"
#include "packs/rigComponent/src/rig.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigSystems/src/rigSystems.h"

namespace {

using rigkit::ecs::CDrawStyle;

constexpr float kTau = 6.28318530718f;

/// Local units the dial is drawn in. Update scales the root so this circle
/// fits the short side of whatever window the app gets.
constexpr float kDialSpan = 700.f;

CDrawStyle inkLine(float alpha, float width) {
	return rig::stroke(0.11f, 0.13f, 0.18f, alpha, width);
}

CDrawStyle inkDisc(float alpha) {
	return rig::fill(0.11f, 0.13f, 0.18f, alpha);
}

CDrawStyle accentDisc() {
	return rig::fill(0.84f, 0.29f, 0.20f);
}

/**
 * Circle sitting `distance` along the parent's local x axis.
 *
 * The transform carries the offset, because that is what the parent turns.
 * Shape coordinates ride the parent's translation and scale but not its turn,
 * so the circle stays centred on its own origin and orbits by transform alone.
 */
entt::entity circleAt(entt::entity parent, float distance, float radius, const CDrawStyle& style,
					  const std::string& name) {
	auto* ecs = rig::currentEcs();
	if (!ecs) {
		return entt::null;
	}
	auto e = rig::makeCircle(0.f, 0.f, radius, style, name);
	rig::parentTo(e, parent);
	ecs->getComponent<rigkit::ecs::CTransform>(e).position = {distance, 0.f, 0.f};
	return e;
}

} // namespace

void PresentApp::setup() {
	spdlog::info("present: rigSystems hierarchy + shape present");
	m_engine->setClearColor(0.94f, 0.93f, 0.90f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->initAll();
	packs->setupAll();

	// Everything hangs off one root. Update writes its position and scale;
	// SHierarchy folds that into the world matrix of every part below.
	m_root = rig::makeGroup({0.f, 0.f, 0.f}, "dial-root");

	// Fixed plate: rings and ticks in root-local coordinates, so they follow
	// the root without ever being touched again.
	for (float radius : {96.f, 168.f, 240.f, 300.f}) {
		circleAt(m_root, 0.f, radius, inkLine(0.28f, 1.5f), "ring-" + std::to_string(int(radius)));
	}
	for (int i = 0; i < 24; ++i) {
		const float a = kTau * float(i) / 24.f;
		const float inner = 300.f;
		const float outer = (i % 6 == 0) ? 328.f : 314.f;
		auto tick = rig::makeLine(std::cos(a) * inner, std::sin(a) * inner, std::cos(a) * outer,
								  std::sin(a) * outer, inkLine(0.55f, 2.f),
								  "tick-" + std::to_string(i));
		rig::parentTo(tick, m_root);
	}
	circleAt(m_root, 0.f, 7.f, inkDisc(1.f), "hub");

	// Four arms, each a single turning transform. What hangs below an arm is
	// plain data — the arm is the only thing Update moves.
	struct ArmSpec {
		float radius;
		float speed;
		float phase;
		float bob;
		bool accent;
		float moonSpeed;
		float moonReach;
	};
	const ArmSpec arms[] = {
		{276.f, 0.20f, 4.86f, 17.f, true, 1.30f, 46.f},
		{240.f, -0.34f, 3.86f, 13.f, true, 1.70f, 30.f},
		{168.f, 0.55f, 4.85f, 10.f, false, 0.f, 0.f},
		{96.f, -0.90f, 1.25f, 8.f, false, 0.f, 0.f},
	};

	int index = 0;
	for (const auto& arm : arms) {
		const std::string id = std::to_string(index++);
		auto turn = rig::makeGroup({0.f, 0.f, 0.f}, "arm-" + id);
		rig::parentTo(turn, m_root);
		m_turns.push_back({turn, arm.speed, arm.phase});

		// A dotted ray out to the head: six circles at six distances, all under
		// the one turn. Depth is what makes them swing together.
		for (int step = 1; step <= 6; ++step) {
			const float t = float(step) / 7.f;
			circleAt(turn, arm.radius * t, 1.6f + 2.6f * t, inkDisc(0.55f),
					 "arm-" + id + "-dot-" + std::to_string(step));
		}

		circleAt(turn, arm.radius, arm.bob, arm.accent ? accentDisc() : inkDisc(0.9f),
				 "arm-" + id + "-bob");
		if (arm.accent) {
			circleAt(turn, arm.radius, arm.bob + 11.f, inkLine(0.75f, 2.5f), "arm-" + id + "-halo");
		}

		if (arm.moonSpeed != 0.f) {
			// Second turn on top of the first: the moon reads the arm's world
			// through SHierarchy, so it orbits a head that is itself orbiting.
			auto moonTurn = rig::makeGroup({arm.radius, 0.f, 0.f}, "arm-" + id + "-moon-turn");
			rig::parentTo(moonTurn, turn);
			m_turns.push_back({moonTurn, arm.moonSpeed, arm.phase * 1.7f});
			circleAt(turn, arm.radius, arm.moonReach, inkLine(0.20f, 1.f),
					 "arm-" + id + "-moon-path");
			circleAt(moonTurn, arm.moonReach, 4.5f, inkDisc(0.9f), "arm-" + id + "-moon");
		}
	}
}

void PresentApp::update(float dt) {
	m_time += dt;
	auto* ecs = m_engine->getECSManager();
	if (!ecs || m_root == entt::null) {
		return;
	}

	// One write centres and fits the whole dial: the root is the only part
	// that knows about the window.
	int designW = 0, designH = 0, fbW = 0, fbH = 0;
	m_engine->getPresentSize(designW, designH, fbW, fbH);
	auto& root = ecs->getComponent<rigkit::ecs::CTransform>(m_root);
	root.position = {designW * 0.5f, designH * 0.5f, 0.f};
	const float fit = float(std::min(designW, designH)) / kDialSpan;
	root.scale = {fit, fit, 1.f};

	for (const auto& turn : m_turns) {
		if (!ecs->hasComponent<rigkit::ecs::CTransform>(turn.entity)) {
			continue;
		}
		auto& t = ecs->getComponent<rigkit::ecs::CTransform>(turn.entity);
		t.setEulerRadians({0.f, 0.f, turn.phase + turn.speed * m_time});
	}
}
