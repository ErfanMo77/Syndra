#include "lpch.h"
#include "ScenePanel.h"
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
		int size = m_Shaders.GetShaders().size();
		int index = 0;
		for (auto&& [name, shader] : m_Shaders.GetShaders())
		{
			m_ShaderNames.push_back(name);
		}
		m_EmptyTexture = Texture2D::Create("assets/Models/cube/default.png");
		m_TextureId = reinterpret_cast<void*>(m_EmptyTexture->GetRendererID());
		m_SelectedShader = "diffuse";
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

	void ScenePanel::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont =io.Fonts->Fonts[1];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 5 });
		ImGui::Text(label.c_str());
		ImGui::PopStyleVar();
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 5 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	template<typename T>
	static bool DrawComponent(const std::string& name, Entity entity, bool removable, bool* removed)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
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

	void ScenePanel::DrawComponents(Entity entity)
	{
		ImGui::Separator();

		static bool TagRemoved = false;
		if (DrawComponent<TagComponent>("Tag", entity, false, &TagRemoved)) {
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
		if (DrawComponent<TransformComponent>("Transform", entity, false, &TransformRemoved)) {
			auto& component = entity.GetComponent<TransformComponent>();
			ImGui::Separator();
			DrawVec3Control("Translation", component.Translation);
			glm::vec3 Rot = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", Rot);
			component.Rotation = glm::radians(Rot);
			DrawVec3Control("Scale", component.Scale, 1.0f);
			ImGui::TreePop();
		}

		static bool MeshRemoved = false;
		if (DrawComponent<MeshComponent>("Mesh", entity, true, &MeshRemoved)) {
			ImGui::Separator();
			auto& tag = entity.GetComponent<MeshComponent>().path;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, tag.c_str());
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2,5 });
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 5,0 });
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80);
			if (ImGui::InputText("##Path", buffer, sizeof(buffer))) {
				tag = std::string(buffer);
			}
			ImGui::PopStyleVar(2);
			ImGui::SameLine();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5,4 });
			if (ImGui::Button("Open")) {
				auto path = FileDialogs::OpenFile("Syndra Model (*.*)\0*.*\0");
				auto dir = std::filesystem::current_path();
				if (path) {
					std::string filePath;
					if (path->find(dir.string()) != std::string::npos) {
						filePath = path->substr(dir.string().size());
					}
					else {
						filePath = *path;
					}
					tag = filePath;
					entity.GetComponent<MeshComponent>().model = Model(*path);
				}
			}
			ImGui::PopStyleVar();
			ImGui::TreePop();
			if (MeshRemoved) {
				entity.RemoveComponent<MeshComponent>();
				MeshRemoved = false;
			}
		}

		static bool MaterialRemoved = false;
		ImGui::Separator();
		if (DrawComponent<MaterialComponent>("Material", entity, true, &MaterialRemoved))
		{
			auto& component = entity.GetComponent<MaterialComponent>();

			ImGui::Separator();
			//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2,5 });
			//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10,0 });
			//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 80);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
			ImGui::Text("Shaders\0");

			ImGui::PopStyleVar();
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
			static int item_current_idx = 0;                    // Here our selection data is an index.
			const char* combo_label = component.m_Shader->GetName().c_str();				// Label to preview before opening the combo (technically it could be anything)
			if (ImGui::BeginCombo("##Shaders", combo_label))
			{
				for (int n = 0; n < m_ShaderNames.size(); n++)
				{
					const bool is_selected = (item_current_idx == n);

					if (ImGui::Selectable(m_ShaderNames[n].c_str(), is_selected)) {
						item_current_idx = n;
						m_SelectedShader = m_ShaderNames[n];
						component.material = Material::Create(m_Shaders.Get(m_SelectedShader));
						component.m_Shader = m_Shaders.Get(m_SelectedShader);
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();
			ImGui::Columns(1);
			ImGui::Separator();
			ImGuiIO& io = ImGui::GetIO();
			std::vector<Sampler>& samplers = component.material->GetSamplers();
			auto& materialTextures = component.material->GetTextures();
			for (auto& sampler : samplers)
			{			
				ImGui::PushID(sampler.name.c_str());
				int frame_padding = -1 + 0;                             // -1 == uses default padding (style.FramePadding)
				ImVec2 size = ImVec2(64.0f,64.0f);                     // Size of the image we want to make visible
				ImGui::Text(sampler.name.c_str());
				m_TextureId = reinterpret_cast<void*>(m_EmptyTexture->GetRendererID());
				auto& texture = component.material->GetTexture(sampler);
				if (texture)
				{
					m_TextureId = reinterpret_cast<void*>(texture->GetRendererID());
				}

				ImGui::SameLine();
				ImGui::Checkbox("Use", &sampler.isUsed);

				if (ImGui::ImageButton(m_TextureId, size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 })) {

					auto path = FileDialogs::OpenFile("Syndra Texture (*.*)\0*.*\0");
					if (path) {
						materialTextures[sampler.binding] = Texture2D::Create(*path);
					}
				}
				ImGui::PopID();
				

			}
			ImGui::TreePop();

			if (MaterialRemoved) {
				entity.RemoveComponent<MaterialComponent>();
				MaterialRemoved = false;
			}
			ImGui::Separator();
		}

		static bool CameraRemoved = false;
		if (DrawComponent<CameraComponent>("Camera", entity, true, &CameraRemoved))
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
				if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
					camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));

				float perspectiveNear = camera.GetPerspectiveNearClip();
				if (ImGui::DragFloat("Near", &perspectiveNear))
					camera.SetPerspectiveNearClip(perspectiveNear);

				float perspectiveFar = camera.GetPerspectiveFarClip();
				if (ImGui::DragFloat("Far", &perspectiveFar))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (ImGui::DragFloat("Size", &orthoSize))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if (ImGui::DragFloat("Near", &orthoNear))
					camera.SetOrthographicNearClip(orthoNear);

				float orthoFar = camera.GetOrthographicFarClip();
				if (ImGui::DragFloat("Far", &orthoFar))
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
					m_SelectionContext.AddComponent<MaterialComponent>(m_Shaders.Get("diffuse"));
				else
					SN_CORE_WARN("This entity already has the Camera Component!");
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::PopItemWidth();



	}

	

}