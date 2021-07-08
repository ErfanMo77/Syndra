#pragma once

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"
#include "Engine/Core/Input.h"
#include "Engine/ImGui/IconsFontAwesome5.h"

#include "Panels/MaterialPanel.h"
#include "Panels/MeshPanel.h"
#include "Panels/LightPanel.h"
#include "Panels/CameraPanel.h"
#include "Panels/ContentBrowser.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Syndra {

	class ScenePanel {

	public:
		ScenePanel() = default;
		ScenePanel(const Ref<Scene>& scene);
		void SetContext(const Ref<Scene>& scene);
		void OnImGuiRender(bool* sceneHierarchyOpen, bool* propertiesOpen);

		Entity GetSelectedEntity() const { return m_SelectionContext; }

		void SetSelectedEntity(Entity entity){ m_SelectionContext = entity; }

		void CreateDuplicate();
	private:
		void DrawEntity(Ref<Entity>& entity);
		void DrawComponents(Entity& entity);

	private:
		std::vector<std::string> m_ShaderNames;

		Ref<MaterialPanel> m_MaterialPanel;
		Ref<MeshPanel> m_MeshPanel;
		Ref<LightPanel> m_LightPanel;
		Ref<CameraPanel> m_CameraPanel;
		Ref<ContentBrowser> m_ContentBrowser;

		Ref<Scene> m_Context;
		Entity m_SelectionContext;
		ShaderLibrary m_Shaders;
		bool m_EntityCreated = false;

	};

}