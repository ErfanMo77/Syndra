#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "../UI.h"

namespace Syndra {

	class LightPanel {

	public:
		LightPanel();
		~LightPanel() = default;

		void DrawLight(Entity& entity);

	};

}