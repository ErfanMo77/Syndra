#pragma once
#include "entt.hpp"
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/PerspectiveCamera.h"

namespace Syndra {

	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());

		void DestroyEntity(Entity entity);
		Entity FindEntity(uint32_t id);

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts, PerspectiveCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);
	
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	private:
		entt::registry m_Registry;

		std::vector<Ref<Entity>> m_Entities;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		friend class Entity;
		friend class ScenePanel;
		friend class SceneSerializer;
	};

}

