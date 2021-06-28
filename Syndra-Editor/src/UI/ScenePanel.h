#pragma once

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"
#include "Engine/Core/Input.h"
#include "Panels/MaterialPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Syndra {

	class ScenePanel {

	public:
		ScenePanel() = default;
		ScenePanel(const Ref<Scene>& scene);
		void SetContext(const Ref<Scene>& scene);
		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_SelectionContext; }

		void SetSelectedEntity(Entity entity){ m_SelectionContext = entity; }
	private:
		void DrawEntity(Ref<Entity>& entity);
		void DrawComponents(Entity& entity);

	private:
		std::vector<std::string> m_ShaderNames;

		Ref<MaterialPanel> m_MaterialPanel;
	
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
		ShaderLibrary m_Shaders;
		bool m_EntityCreated = false;

	};

}