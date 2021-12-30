#pragma once
#include "entt.hpp"
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/SceneRenderer.h"

namespace Syndra {

	class Entity;

	enum class PrimitiveType
	{
		Cube,
		Plane,
		Sphere
	};

	class Scene
	{
	public:
		Scene(const std::string& name = "Untitled");
		Scene(const Scene& other) = default;
		~Scene();

		Ref<Entity> CreateEntity(const std::string& name = std::string());
		Ref<Entity> CreateEntity(const Entity& entity);

		//Creating entities with default components
		Ref<Entity> CreatePrimitive(PrimitiveType type);
		Ref<Entity> CreateLight(LightType type);

		void DestroyEntity(const Entity& entity);
		Entity FindEntity(uint32_t id);

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts);
		void OnViewportResize(uint32_t width, uint32_t height);
		void OnCameraUpdate(Timestep ts) { m_Camera->OnUpdate(ts); }

		uint32_t GetMainTextureID() { return SceneRenderer::GetTextureID(0); }
		Ref<FrameBuffer> GetMainFrameBuffer() { return SceneRenderer::GetGeoFrameBuffer(); }
		FramebufferSpecification GetSpec() { return SceneRenderer::GetMainFrameSpec(); }

		ShaderLibrary& GetShaderLibrary() { return m_Shaders; }
		void SetShaderLibrary(const ShaderLibrary& shaders) { m_Shaders = shaders; }

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	private:
		entt::registry m_Registry;

		std::vector<Ref<Entity>> m_Entities;
		std::string m_EnvironmentPath;
		
		std::string m_Name;

		PerspectiveCamera* m_Camera;
		ShaderLibrary m_Shaders;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		friend class Entity;
		friend class EditorLayer;
		friend class ScenePanel;
		friend class SceneSerializer;
		friend class SceneRenderer;
	};

}

