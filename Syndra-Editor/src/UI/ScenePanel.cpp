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
	}

	void ScenePanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
	}

	void ScenePanel::OnImGuiRender()
	{

		ImGui::ShowDemoWindow();
		
		ImGui::Begin("Style editor");
		ImGui::ShowStyleEditor();
		ImGui::End();
		//---------------------------------------------Scene hierarchy-------------------------------//
		ImGui::Begin("Scene hierarchy");

		for (auto ent:m_Context->m_Entities)
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
			if (ImGui::MenuItem("Create empty entity")) {
				m_SelectionContext = *m_Context->CreateEntity();
			}
			if (ImGui::BeginMenu("Add primitive")) {
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
			if (ImGui::BeginMenu("Add Light")) {
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
		//-------------------------------------------Properties--------------------------------------//
		ImGui::Begin("Properties");

		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}

		ImGui::End();
	}


	void ScenePanel::DrawEntity(Ref<Entity>& entity)
	{
		auto& tag = entity->GetComponent<TagComponent>();
		ImGuiTreeNodeFlags flags = ((m_SelectionContext == *entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)*entity, flags, tag.Tag.c_str());

		if (ImGui::IsItemClicked()) {
			m_SelectionContext = *entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete entity")) {
				entityDeleted = true;
			}

			if (ImGui::MenuItem("Duplicate entity")) {
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
		if (UI::DrawComponent<TagComponent>("Tag", entity, false, &TagRemoved)) {
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
		if (UI::DrawComponent<TransformComponent>("Transform", entity, false, &TransformRemoved)) {
			auto& component = entity.GetComponent<TransformComponent>();
			ImGui::Separator();
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

		static bool CameraRemoved = false;
		if (UI::DrawComponent<CameraComponent>("Camera", entity, true, &CameraRemoved))
		{
			auto& component = entity.GetComponent<CameraComponent>();
			auto& camera = component.Camera;

			ImGui::Checkbox("Primary", &component.Primary);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjectionType((SceneCamera::ProjectionType)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if(UI::DragFloat("Vertical FOV", &perspectiveVerticalFov,0.5F,10,189.0f))
					camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));
					
				float perspectiveNear = camera.GetPerspectiveNearClip();
				if(UI::DragFloat("Near", &perspectiveNear,0.1f,0.001f,100.0f))
					camera.SetPerspectiveNearClip(perspectiveNear);

				float perspectiveFar = camera.GetPerspectiveFarClip();
				if (UI::DragFloat("Far", &perspectiveFar, 0.1f, 100.0f, 10000.0f))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (UI::DragFloat("Size", &orthoSize, 0.5f, 0.01f, 100.0f))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if (UI::DragFloat("Near", &orthoNear, 0.5f, 0.01f, 100.0f))
					camera.SetOrthographicNearClip(orthoNear);

				float orthoFar = camera.GetOrthographicFarClip();
				if (UI::DragFloat("Far", &orthoFar, 0.5f, 0.01f, 100.0f))
					camera.SetOrthographicFarClip(orthoFar);

				ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			}
			ImGui::Separator();
			ImGui::TreePop();

			if (CameraRemoved) {
				entity.RemoveComponent<CameraComponent>();
				CameraRemoved = false;
			}
		}
	

		
		float buttonSz = 100;
		ImGui::PushItemWidth(buttonSz);

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