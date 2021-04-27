#include "lpch.h"
#include "Engine/Scene/Scene.h"

#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"

namespace Syndra {

	Scene::Scene()
	{
		Entity::s_Scene = this;
		SceneRenderer::Initialize();
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create() };
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		entity.AddComponent<TransformComponent>();
		m_Entities.push_back(CreateRef<Entity>(entity));
		return entity;
	}

	void Scene::DestroyEntity(const Entity& entity)
	{
		for (auto& e : m_Entities) {
			if (*e == entity) {
				auto it = std::find(m_Entities.begin(), m_Entities.end(), e);
				if (it != m_Entities.end()) {
					m_Entities.erase(it);
					break;
				}
			}
		}
		m_Registry.destroy(entity);
	}

	Entity Scene::FindEntity(uint32_t id)
	{
		for (auto& e : m_Entities) {
			if (*e == (entt::entity)id) {
				return *e;
			}
		}
		return {};
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{

	}

	void Scene::OnUpdateEditor(Timestep ts, PerspectiveCamera& camera)
	{
		SceneRenderer::BeginScene(camera);
		auto view = m_Registry.view<TransformComponent,MeshComponent>();
		for (auto ent : view)
		{
			auto& tc = view.get<TransformComponent>(ent);
			auto& mc = view.get<MeshComponent>(ent);
			//TODO material
			//Entity entity = { ent };
			//entity.SetSelected(true);
			//auto entity = this->FindEntity((uint32_t)ent);
			SceneRenderer::RenderEntity(ent, tc, mc);
		}

		SceneRenderer::EndScene();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
		SceneRenderer::OnViewPortResize(width, height);
		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<MeshComponent>(Entity entity, MeshComponent& component)
	{
	}

}
