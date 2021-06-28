#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <string>

#include "Engine/Scene/Entity.h"

#include <glm/gtc/type_ptr.hpp>

namespace Syndra {

	class UI {

	public:

		static bool SliderFloat(const std::string& name, float* value, float min = 0.0f, float max = 1.0f);
		static bool DragFloat(const std::string& name, float* value, float speed = 0.1f, float min = 0.0f, float max = 100.0f);

		static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

		template<typename T>
		static bool DrawComponent(const std::string& name, Entity entity, bool removable, bool* removed)
		{
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
			if (entity.HasComponent<T>())
			{
				auto& component = entity.GetComponent<T>();
				ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
				bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
				ImGui::PopStyleVar();
				ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
				if (ImGui::Button("...", ImVec2{ lineHeight, lineHeight }))
				{
					ImGui::OpenPopup("ComponentSettings");
				}

				bool removeComponent = false;
				if (ImGui::BeginPopup("ComponentSettings"))
				{
					if (removable) {
						if (ImGui::MenuItem("Remove component"))
							removeComponent = true;
					}
					ImGui::EndPopup();
				}

				if (removeComponent)
					*removed = true;

				return open;
			}
			return false;
		}
	};

}