#pragma once
#include "entt.hpp"
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/SceneRenderer.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <utility>

namespace Syndra {

	class Entity;
	class Material;

	enum class PrimitiveType
	{
		Cube,
		Plane,
		Sphere
	};

	class Scene
	{
	public:
		struct SceneMaterialRecord
		{
			std::string Name;
			Ref<Material> MaterialRef;
		};

		Scene(const std::string& name = "Untitled");
		Scene(const Scene& other) = delete;
		Scene& operator=(const Scene& other) = delete;
		Scene(Scene&& other) = delete;
		Scene& operator=(Scene&& other) = delete;
		~Scene() = default;

		Ref<Entity> CreateEntity(const std::string& name = std::string());
		Ref<Entity> CreateEntity(const Entity& entity);

		//Creating entities with default components
		Ref<Entity> CreatePrimitive(PrimitiveType type);
		Ref<Entity> CreateLight(LightType type);

		void DestroyEntity(const Entity& entity);
		Entity FindEntity(uint32_t id);
		void SetParent(const Entity& child, const Entity& parent);
		void Unparent(const Entity& child);
		Entity GetParent(const Entity& entity) const;
		glm::mat4 GetWorldTransform(const Entity& entity) const;
		glm::vec3 GetWorldTranslation(const Entity& entity) const;

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts);
		void OnViewportResize(uint32_t width, uint32_t height);
		void OnCameraUpdate(Timestep ts) { m_Camera->OnUpdate(ts); }

		uint64_t RegisterMaterial(const Ref<Material>& material, const std::string& name = "Material");
		Ref<Material> GetMaterial(uint64_t materialId) const;
		uint64_t CloneMaterial(uint64_t materialId, const std::string& newName = "Material Instance");
		void AssignMaterial(Entity entity, uint64_t materialId);
		std::vector<std::pair<uint64_t, std::string>> GetMaterialList() const;
		uint32_t GetMaterialUsageCount(uint64_t materialId);
		void PruneUnusedMaterials();

		uint32_t GetMainTextureID() { return SceneRenderer::GetTextureID(0); }
		Ref<FrameBuffer> GetMainFrameBuffer() { return SceneRenderer::GetMainFrameBuffer(); }
		FramebufferSpecification GetSpec() { return SceneRenderer::GetMainFrameSpec(); }

		ShaderLibrary& GetShaderLibrary() { return m_Shaders; }
		void SetShaderLibrary(const ShaderLibrary& shaders) { m_Shaders = shaders; }

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
		void DestroyEntityRecursive(const Entity& entity);
		bool HasRenderableResourcesInHierarchy(const Entity& entity) const;
		void ProcessPendingEntityDestruction();

	private:
		entt::registry m_Registry;

		std::vector<Ref<Entity>> m_Entities;
		std::vector<entt::entity> m_EntitiesPendingDestroy;
		std::string m_EnvironmentPath;

		std::string m_Name;

		Scope<PerspectiveCamera> m_Camera;
		ShaderLibrary m_Shaders;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;
		uint64_t m_NextMaterialId = 1;
		std::unordered_map<uint64_t, SceneMaterialRecord> m_Materials;

		friend class Entity;
		friend class EditorLayer;
		friend class ScenePanel;
		friend class SceneSerializer;
		friend class SceneRenderer;
		friend class DeferredRenderer;
		friend class ForwardPlusRenderer;
		friend class VulkanDeferredRenderer;
	};

}

