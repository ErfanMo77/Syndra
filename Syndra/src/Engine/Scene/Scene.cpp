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
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
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
			auto tag = view.get<TagComponent>(ent);
			//SN_CORE_TRACE("Entity with ID :{0} and tag name : {1}", ent, tag.Tag);
		}
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

}
