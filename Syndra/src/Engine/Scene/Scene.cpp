#include "lpch.h"
#include "Engine/Scene/Scene.h"

#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"
#include "Engine/Core/Instrument.h"
#include "Engine/Renderer/RenderCommand.h"

#include <algorithm>

namespace Syndra {

	namespace
	{
		void RemoveChildReference(RelationshipComponent& relationship, entt::entity child)
		{
			auto it = std::remove(relationship.Children.begin(), relationship.Children.end(), child);
			if (it != relationship.Children.end())
				relationship.Children.erase(it, relationship.Children.end());
		}
	}

	Scene::Scene(const std::string& name)
		:m_Name(name)
	{
		Entity::s_Scene = this;
		m_Shaders = SceneRenderer::GetShaderLibrary();
		m_Camera = new PerspectiveCamera(45.0f, 1.66f, 0.1f, 1000.0f);
	}

	Scene::~Scene()
	{
		delete m_Camera;
	}

	Ref<Entity> Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create() };
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" + std::to_string(uint32_t(entity)) : name;
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<RelationshipComponent>();
		Ref<Entity> ent = CreateRef<Entity>(entity);
		m_Entities.push_back(ent);
		return ent;
	}

	Ref<Entity> Scene::CreateEntity(const Entity& other)
	{
		Entity entity = { m_Registry.create() };
		Ref<Entity> ent = CreateRef<Entity>(entity);
		ent->AddComponent<TagComponent>(other.GetComponent<TagComponent>());
		ent->AddComponent<TransformComponent>(other.GetComponent<TransformComponent>());
		ent->AddComponent<RelationshipComponent>();
		if (other.HasComponent<CameraComponent>()) {
			ent->AddComponent<CameraComponent>(other.GetComponent<CameraComponent>());
		}
		if (other.HasComponent<MeshComponent>()) {
			ent->AddComponent<MeshComponent>(other.GetComponent<MeshComponent>());
		}
		if (other.HasComponent<MaterialComponent>()) {
			ent->AddComponent<MaterialComponent>(other.GetComponent<MaterialComponent>());
		}
		if (other.HasComponent<LightComponent>()) {
			ent->AddComponent<LightComponent>(other.GetComponent<LightComponent>());
		}
		m_Entities.push_back(ent);
		return ent;
	}

	Ref<Entity> Scene::CreatePrimitive(PrimitiveType type)
	{
		auto ent = this->CreateEntity();
		std::string path;
		switch (type)
		{
		case Syndra::PrimitiveType::Cube:
			ent->GetComponent<TagComponent>().Tag = "Cube";
			path = std::string("assets\\Models\\cube\\cube.obj");
			ent->AddComponent<MeshComponent>(path);
			break;
		case Syndra::PrimitiveType::Plane:
			ent->GetComponent<TagComponent>().Tag = "Plane";
			path = std::string("assets\\Models\\plane\\plane.obj");
			ent->AddComponent<MeshComponent>(path);
			break;
		case Syndra::PrimitiveType::Sphere:
			ent->GetComponent<TagComponent>().Tag = "Sphere";
			path = std::string("assets\\Models\\Sphere\\Sphere.fbx");
			ent->AddComponent<MeshComponent>(path);
			break;
		default:
			break;
		}
		return ent;
	}

	Syndra::Ref<Syndra::Entity> Scene::CreateLight(LightType type)
	{
		auto ent = this->CreateEntity();
		std::string path;
		Ref<Light> light;
		switch (type)
		{
		case Syndra::LightType::Directional:
			ent->GetComponent<TagComponent>().Tag = "Directional Light";
			light = CreateRef<DirectionalLight>(glm::vec3(1.0f));
			ent->AddComponent<LightComponent>(type, light);
			break;
		case Syndra::LightType::Point:
			ent->GetComponent<TagComponent>().Tag = "Point Light";
			light = CreateRef<PointLight>(glm::vec3(1.0f));
			ent->AddComponent<LightComponent>(type, light);
			break;
		case Syndra::LightType::Spot:
			ent->GetComponent<TagComponent>().Tag = "Spot Light";
			light = CreateRef<SpotLight>(glm::vec3(1.0f));
			ent->AddComponent<LightComponent>(type, light);
			break;
		case Syndra::LightType::Area:
			//TODO
			break;
		default:
			break;
		}
		return ent;
	}

	void Scene::DestroyEntity(const Entity& entity)
	{
		if (!entity || !m_Registry.valid(entity))
			return;

		const entt::entity handle = static_cast<entt::entity>(entity);
		const auto it = std::find(m_EntitiesPendingDestroy.begin(), m_EntitiesPendingDestroy.end(), handle);
		if (it == m_EntitiesPendingDestroy.end())
			m_EntitiesPendingDestroy.push_back(handle);
	}

	void Scene::DestroyEntityRecursive(const Entity& entity)
	{
		if (!entity || !m_Registry.valid(entity))
			return;

		if (m_Registry.has<RelationshipComponent>(entity))
		{
			auto& relationship = m_Registry.get<RelationshipComponent>(entity);
			const auto children = relationship.Children;
			for (entt::entity child : children)
			{
				if (child != entt::null && m_Registry.valid(child))
					DestroyEntityRecursive(Entity{ child });
			}

			if (relationship.Parent != entt::null && m_Registry.valid(relationship.Parent) && m_Registry.has<RelationshipComponent>(relationship.Parent))
			{
				auto& parentRelationship = m_Registry.get<RelationshipComponent>(relationship.Parent);
				RemoveChildReference(parentRelationship, entity);
			}
		}

		m_Registry.destroy(entity);
		m_Entities.erase(
			std::remove_if(
				m_Entities.begin(),
				m_Entities.end(),
				[&entity](const Ref<Entity>& item)
				{
					return item && *item == entity;
				}),
			m_Entities.end());
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

	bool Scene::HasRenderableResourcesInHierarchy(const Entity& entity) const
	{
		if (!entity || !m_Registry.valid(entity))
			return false;

		if (m_Registry.has<MeshComponent>(entity) || m_Registry.has<MaterialComponent>(entity))
			return true;

		const RelationshipComponent* relationship = m_Registry.try_get<RelationshipComponent>(entity);
		if (relationship == nullptr)
			return false;

		for (entt::entity child : relationship->Children)
		{
			if (child == entt::null || !m_Registry.valid(child))
				continue;

			if (HasRenderableResourcesInHierarchy(Entity{ child }))
				return true;
		}

		return false;
	}

	void Scene::ProcessPendingEntityDestruction()
	{
		if (m_EntitiesPendingDestroy.empty())
			return;

		bool requiresGpuIdle = false;
		for (entt::entity pendingEntity : m_EntitiesPendingDestroy)
		{
			if (pendingEntity == entt::null || !m_Registry.valid(pendingEntity))
				continue;

			if (HasRenderableResourcesInHierarchy(Entity{ pendingEntity }))
			{
				requiresGpuIdle = true;
				break;
			}
		}

		if (requiresGpuIdle)
			RenderCommand::WaitForIdle();

		const std::vector<entt::entity> pendingEntities = m_EntitiesPendingDestroy;
		m_EntitiesPendingDestroy.clear();
		for (entt::entity pendingEntity : pendingEntities)
		{
			if (pendingEntity == entt::null || !m_Registry.valid(pendingEntity))
				continue;

			DestroyEntityRecursive(Entity{ pendingEntity });
		}
	}

	void Scene::SetParent(const Entity& child, const Entity& parent)
	{
		if (!child || !parent || child == parent)
			return;
		if (!m_Registry.valid(child) || !m_Registry.valid(parent))
			return;
		if (!m_Registry.has<RelationshipComponent>(child) || !m_Registry.has<RelationshipComponent>(parent))
			return;

		// Prevent cycles by rejecting parent assignments to descendants of the child.
		Entity ancestor = parent;
		while (ancestor)
		{
			if (ancestor == child)
				return;

			if (!m_Registry.has<RelationshipComponent>(ancestor))
				break;
			ancestor = Entity{ m_Registry.get<RelationshipComponent>(ancestor).Parent };
		}

		Unparent(child);

		auto& childRelationship = m_Registry.get<RelationshipComponent>(child);
		auto& parentRelationship = m_Registry.get<RelationshipComponent>(parent);
		childRelationship.Parent = static_cast<entt::entity>(parent);
		parentRelationship.Children.push_back(static_cast<entt::entity>(child));
	}

	void Scene::Unparent(const Entity& child)
	{
		if (!child || !m_Registry.valid(child))
			return;
		if (!m_Registry.has<RelationshipComponent>(child))
			return;

		auto& childRelationship = m_Registry.get<RelationshipComponent>(child);
		if (childRelationship.Parent == entt::null)
			return;

		if (m_Registry.valid(childRelationship.Parent) && m_Registry.has<RelationshipComponent>(childRelationship.Parent))
		{
			auto& parentRelationship = m_Registry.get<RelationshipComponent>(childRelationship.Parent);
			RemoveChildReference(parentRelationship, static_cast<entt::entity>(child));
		}

		childRelationship.Parent = entt::null;
	}

	Entity Scene::GetParent(const Entity& entity) const
	{
		if (!entity || !m_Registry.valid(entity))
			return {};
		if (!m_Registry.has<RelationshipComponent>(entity))
			return {};

		entt::entity parent = m_Registry.get<RelationshipComponent>(entity).Parent;
		if (parent == entt::null || !m_Registry.valid(parent))
			return {};

		return Entity{ parent };
	}

	glm::mat4 Scene::GetWorldTransform(const Entity& entity) const
	{
		if (!entity || !m_Registry.valid(entity) || !m_Registry.has<TransformComponent>(entity))
			return glm::mat4(1.0f);

		glm::mat4 transform = m_Registry.get<TransformComponent>(entity).GetTransform();
		Entity parent = GetParent(entity);
		while (parent)
		{
			if (!m_Registry.has<TransformComponent>(parent))
				break;

			transform = m_Registry.get<TransformComponent>(parent).GetTransform() * transform;
			parent = GetParent(parent);
		}

		return transform;
	}

	glm::vec3 Scene::GetWorldTranslation(const Entity& entity) const
	{
		const glm::mat4 world = GetWorldTransform(entity);
		return glm::vec3(world[3]);
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		ProcessPendingEntityDestruction();

	}

	void Scene::OnUpdateEditor(Timestep ts)
	{
		SN_PROFILE_SCOPE("Scene::OnUpdateEditor");
		ProcessPendingEntityDestruction();
		SceneRenderer::BeginScene(*m_Camera);
		SceneRenderer::RenderScene();
		SceneRenderer::EndScene();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
		m_Camera->SetViewportSize(width, height);
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

	std::string LightTypeToLightName(LightType type) {
		if (type == LightType::Directional)
			return "Directional";
		if (type == LightType::Point)
			return "Point";
		if (type == LightType::Spot)
			return "Spot";
		if (type == LightType::Area)
			return "Area";
		return "";
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		//static_assert(false);
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

	template<>
	void Scene::OnComponentAdded<MaterialComponent>(Entity entity, MaterialComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<LightComponent>(Entity entity, LightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RelationshipComponent>(Entity entity, RelationshipComponent& component)
	{
	}

}
