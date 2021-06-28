#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <string>

#include <glm/gtc/type_ptr.hpp>

namespace Syndra {

	class UI {

	public:

		static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
		static bool SliderFloat(const std::string& name, float* value, float min=0.0f, float max=1.0f);
		static bool DragFloat(const std::string& name, float* value, float speed = 0.1f, float min=0.0f, float max=100.0f);

	};

}