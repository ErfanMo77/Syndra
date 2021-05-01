#pragma once
#include "entt.hpp"
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/SceneRenderer.h"

namespace Syndra {

	class Entity;

	class Scene
	{
	public:
		Scene();
		Scene(const Scene& other) = default;
		~Scene();

		Ref<Entity> CreateEntity(const std::string& name = std::string());
		Ref<Entity> CreateEntity(const Entity& entity);

		void DestroyEntity(const Entity& entity);
		Entity FindEntity(uint32_t id);

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts, PerspectiveCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);
		void ReloadShader() { SceneRenderer::Reload(); }

		uint32_t GetMainTextureID() { return SceneRenderer::GetTextureID(0); }
		Ref<FrameBuffer> GetMouseFrameBuffer() { return SceneRenderer::GetMouseFrameBuffer(); }
		FramebufferSpecification GetSpec() { return SceneRenderer::GetMainFrameSpec(); }

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
		friend class SceneRenderer;
	};

}

