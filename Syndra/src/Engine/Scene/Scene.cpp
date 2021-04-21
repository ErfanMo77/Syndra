#include "lpch.h"
#include "Scene.h"

#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"

namespace Syndra {

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		entity.AddComponent<TransformComponent>();
		m_Entities.push_back(entity);
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		for (auto& e : m_Entities) {
			if (e == entity) {
				auto it = std::find(m_Entities.begin(), m_Entities.end(), e);
				if (it != m_Entities.end()) {
					m_Entities.erase(it);
					break;
				}
			}
		}
		m_Registry.destroy(entity);
	}

	entt::entity Scene::FindEntity(uint32_t id)
	{
		for (auto& e : m_Entities) {
			if (e == (entt::entity)id) {
				return e;
			}
		}
		return {};
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{

	}

	void Scene::OnUpdateEditor(Timestep ts, PerspectiveCamera& camera)
	{
		//TODO
		auto view = m_Registry.view<TransformComponent,TagComponent>();
		for (auto ent : view)
		{
			auto& tag = view.get<TagComponent>(ent);
			//SN_CORE_TRACE("Entity with ID :{0} and tag name : {1}", ent, tag.Tag);
		}
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

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


}
