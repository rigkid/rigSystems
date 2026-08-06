#pragma once
#include <vector>
#include <entt/entt.hpp>
#include "core/U_core.h"

class PresentApp : public rigkit::IApp {
  public:
	PresentApp() {
		window().width = 800;
		window().height = 600;
		window().title = "rigSystems: Present";
	}
	void setup() override;
	void update(float dt) override;
	void draw() override {}

  private:
	/// A turning joint: Update writes its angle, SHierarchy carries it downward.
	struct Turn {
		entt::entity entity{entt::null};
		float speed = 0.f;
		float phase = 0.f;
	};

	entt::entity m_root{entt::null};
	std::vector<Turn> m_turns;
	float m_time = 0.f;
};
