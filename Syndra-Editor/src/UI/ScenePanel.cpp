#include "lpch.h"
#include "ScenePanel.h"

#include <imgui/imgui.h>

namespace Syndra {

	ScenePanel::ScenePanel(const Ref<Scene>& scene)
	{
		m_Context = scene;
	}

	void ScenePanel::OnImGuiRender()
	{
		ImGui::ShowStyleEditor();
		ImGui::Begin("scene hierarchy");

		m_Context->m_Registry.each([&](auto entityID) {
			Entity entity = { entityID, m_Context.get() };
			DrawEntity(entity);
		});

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

		if (opened) {
			ImGui::TreePop();
		}
		//ImGui::Text("%s", tag.Tag.c_str());
	}

}