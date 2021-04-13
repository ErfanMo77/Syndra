#pragma once

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"

namespace Syndra {

	class ScenePanel {

	public:
		ScenePanel() = default;
		ScenePanel(const Ref<Scene>& scene);
		void SetContext(const Ref<Scene>& scene);
		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_SelectionContext; }

	private:
		void DrawEntity(Entity entity);
		void DrawComponents(Entity entity);

	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

	};

}