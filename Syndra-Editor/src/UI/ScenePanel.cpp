#include "lpch.h"
#include "ScenePanel.h"
#include "UI.h"
#include <filesystem>

#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Renderer/Model.h"
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace Syndra {

	ScenePanel::ScenePanel(const Ref<Scene>& scene)
	{
		SetContext(scene);
		m_Shaders = scene->GetShaderLibrary();
		int index = 0;
		for (auto&& [name, shader] : m_Shaders.GetShaders())
		{
			m_ShaderNames.push_back(name);
		}

		m_MaterialPanel = CreateRef<MaterialPanel>();
		m_MeshPanel = CreateRef<MeshPanel>();
		m_LightPanel = CreateRef<LightPanel>();
		m_CameraPanel = CreateRef<CameraPanel>();
		m_ContentBrowser = CreateRef<ContentBrowser>();
	}

	void ScenePanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
	}

	void ScenePanel::OnImGuiRender(bool* sceneHierarchyOpen, bool* propertiesOpen)
	{
		//Show these tabs only in debug mode
#ifdef SN_DEBUG
		ImGui::ShowDemoWindow();

		ImGui::Begin("Style editor");
		ImGui::ShowStyleEditor();
		ImGui::End();
#endif // SN_DEBUG

		//---------------------------------------------Scene hierarchy-------------------------------//
		if (*sceneHierarchyOpen) {
			ImGui::Begin(ICON_FA_LIST_UL " Scene Hierarchy", sceneHierarchyOpen);

			for (auto ent : m_Context->m_Entities)
			{
				if (ent) {
					DrawEntity(ent);
				}
			}

			if (m_EntityCreated) {
				m_Context->CreateEntity(m_SelectionContext);
				m_EntityCreated = false;
			}

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::MenuItem(ICON_FA_FILE" Create empty entity")) {
					m_SelectionContext = *m_Context->CreateEntity();
				}
				if (ImGui::BeginMenu(ICON_FA_CUBE" Add primitive")) {
					if (ImGui::MenuItem("Add Sphere")) {
						m_SelectionContext = *m_Context->CreatePrimitive(PrimitiveType::Sphere);
					}
					if (ImGui::MenuItem("Add Cube")) {
						m_SelectionContext = *m_Context->CreatePrimitive(PrimitiveType::Cube);
					}
					if (ImGui::MenuItem("Add Plane")) {
						m_SelectionContext = *m_Context->CreatePrimitive(PrimitiveType::Plane);
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu(ICON_FA_LIGHTBULB" Add Light")) {
					if (ImGui::MenuItem("Add Point Light")) {
						m_SelectionContext = *m_Context->CreateLight(LightType::Point);
					}
					if (ImGui::MenuItem("Add Directional Light")) {
						m_SelectionContext = *m_Context->CreateLight(LightType::Directional);
					}
					if (ImGui::MenuItem("Add SpotLight")) {
						m_SelectionContext = *m_Context->CreateLight(LightType::Spot);
					}
					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}

			ImGui::End();
		}
		//-------------------------------------------Properties--------------------------------------//
		if (*propertiesOpen) {
			ImGui::Begin(ICON_FA_SLIDERS_H " Properties", propertiesOpen);

			if (m_SelectionContext)
			{
				DrawComponents(m_SelectionContext);
			}

			ImGui::End();
		}
		//Content Browser
		//m_ContentBrowser->OnImGuiRender();

	}


	void ScenePanel::CreateDuplicate()
	{
		m_EntityCreated = true;
	}

	void ScenePanel::DrawEntity(Ref<Entity>& entity)
	{
		auto& tag = entity->GetComponent<TagComponent>();
		ImGuiTreeNodeFlags flags = ((m_SelectionContext == *entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		const char* name="";
		if (entity->HasComponent<MeshComponent>()) {
			name = ICON_FA_CUBE;
		}
		if (entity->HasComponent<LightComponent>()) {
			name = ICON_FA_LIGHTBULB;
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0,2 });
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)*entity, flags, (std::string(name) + " " + tag.Tag).c_str());
		ImGui::PopStyleVar();
		if (ImGui::IsItemClicked()) {
			m_SelectionContext = *entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem(ICON_FA_TRASH" Delete entity")) {
				entityDeleted = true;
			}

			if (ImGui::MenuItem(ICON_FA_CLONE"  Duplicate entity")) {
				m_SelectionContext = *entity;
				m_EntityCreated = true;
			}

			ImGui::EndPopup();
		}
		if (m_SelectionContext == *entity && Input::IsKeyPressed(Key::Delete)) {
			entityDeleted = true;
		}

		//if (m_SelectionContext == *entity && Input::IsKeyPressed(Key::LeftControl) && Input::IsKeyPressed(Key::D) && !m_EntityCreated) {
		//	m_EntityCreated = true;
		//}

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

		if (entityDeleted)
		{
			if (m_SelectionContext == *entity)
				m_SelectionContext = {};
			m_Context->DestroyEntity(*entity);
		}

	}

	void ScenePanel::DrawComponents(Entity& entity)
	{
		ImGui::Separator();

		static bool TagRemoved = false;
		if (UI::DrawComponent<TagComponent>(ICON_FA_PEN " Tag", entity, false, &TagRemoved)) {
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, tag.c_str());
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2,5 });
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10,0 });
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 2);
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
				tag = std::string(buffer);
			}
			ImGui::PopStyleVar(2);
			ImGui::TreePop();
		}

		ImGui::Separator();

		static bool TransformRemoved = false;
		if (UI::DrawComponent<TransformComponent>(ICON_FA_PENCIL_RULER" Transform", entity, false, &TransformRemoved)) {
			auto& component = entity.GetComponent<TransformComponent>();
			UI::DrawVec3Control("Translation", component.Translation);
			glm::vec3 Rot = glm::degrees(component.Rotation);
			UI::DrawVec3Control("Rotation", Rot);
			component.Rotation = glm::radians(Rot);
			UI::DrawVec3Control("Scale", component.Scale, 1.0f);
			ImGui::TreePop();
		}

		m_MeshPanel->DrawMesh(entity);
		m_MaterialPanel->DrawMaterial(entity);
		m_LightPanel->DrawLight(entity);
		m_CameraPanel->DrawCamera(entity);
		
		float buttonSz = 100;
		ImGui::PushItemWidth(buttonSz);
		ImGui::Separator();
		ImGui::Dummy({0,10});
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetContentRegionAvail().x/2.0f - buttonSz/2.0f );

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 15,5 });
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");
		ImGui::PopStyleVar();

		if (ImGui::BeginPopup("AddComponent"))
		{
			if (ImGui::MenuItem("Camera"))
			{
				if (!m_SelectionContext.HasComponent<CameraComponent>())
					m_SelectionContext.AddComponent<CameraComponent>();
				else
					SN_CORE_WARN("This entity already has the Camera Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Mesh"))
			{
				if (!m_SelectionContext.HasComponent<MeshComponent>())
					m_SelectionContext.AddComponent<MeshComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Material"))
			{
				if (!m_SelectionContext.HasComponent<MaterialComponent>())
					m_SelectionContext.AddComponent<MaterialComponent>(m_Shaders.Get("GeometryPass"));
				else
					SN_CORE_WARN("This entity already has the Camera Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Light"))
			{
				if (!m_SelectionContext.HasComponent<LightComponent>())
					m_SelectionContext.AddComponent<LightComponent>();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		ImGui::PopItemWidth();
	}

}