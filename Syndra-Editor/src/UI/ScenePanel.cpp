#include "lpch.h"
#include "ScenePanel.h"

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace Syndra {

	ScenePanel::ScenePanel(const Ref<Scene>& scene)
	{
		m_Context = scene;
	}

	void ScenePanel::OnImGuiRender()
	{
		ImGui::ShowDemoWindow();

		//---------------------------------------------Scene hierarchy-------------------------------//
		ImGui::Begin("Scene hierarchy");

		for (auto& ent:m_Context->m_Entities)
		{
			DrawEntity(*ent);
		}

		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			m_SelectionContext = {};

		if (ImGui::BeginPopupContextWindow(0, 1, false))
		{
			if (ImGui::Selectable("Create empty entity")) {
				m_Context->CreateEntity();
			}
			ImGui::EndPopup();
		}

		ImGui::End();
		//-------------------------------------------Properties--------------------------------------//
		ImGui::Begin("Properties");
		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
			ImGui::SetNextItemWidth(-1);
			if (ImGui::Button("Add component")) {

			}
			
		}
		ImGui::End();
	}

	void ScenePanel::DrawEntity(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>();
		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.Tag.c_str());

		if (ImGui::IsItemClicked()) {
			m_SelectionContext = entity;
		}

		//for (int n = 0; n < m_Context->m_Entities.size(); n++)
		//{
		//	auto& item = m_Context->m_Entities[n];
		//	if (ImGui::IsItemActive() && !ImGui::IsItemHovered())
		//	{
		//		int n_next = n + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
		//		if (n_next >= 0 && n_next < m_Context->m_Entities.size())
		//		{
		//			m_Context->m_Entities[n] = m_Context->m_Entities[n_next];
		//			m_Context->m_Entities[n_next] = item;
		//			ImGui::ResetMouseDragDelta();
		//		}
		//	}
		//}

		if (opened) {
			ImGui::TreePop();
		}
	}

	void ScenePanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>()) {

			if (ImGui::TreeNodeEx((void*)typeid(TagComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Tag")) {

				auto& tag = entity.GetComponent<TagComponent>().Tag;

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, tag.c_str());
				if (ImGui::InputText("tag", buffer, sizeof(buffer))) {
					tag = std::string(buffer);
				}

				if (ImGui::IsItemClicked()) {

				}
				ImGui::TreePop();
			}
		}
	
		if (entity.HasComponent<TransformComponent>()) {

			if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform")){
				auto& translate = entity.GetComponent<TransformComponent>().Translation;
				auto& scale = entity.GetComponent<TransformComponent>().Scale;
				auto& rot = entity.GetComponent<TransformComponent>().Rotation;


				ImGui::TextColored(ImVec4(.8f, .1f, .5f, 1.0f), "Position");
				ImGui::DragFloat3("", glm::value_ptr(translate), 0.1f);

				ImGui::TextColored(ImVec4(.8f, .1f, .5f, 1.0f), "Rotation");
				ImGui::DragFloat3("1", glm::value_ptr(rot), 0.5f, 0, 180);

				ImGui::TextColored(ImVec4(.8f, .1f, .5f, 1.0f), "Scale");
				ImGui::DragFloat3("2", glm::value_ptr(scale), 0.1f);

				ImGui::TreePop();
			}

		}
	}

}