#pragma once

#include "entt.hpp"
#include "scene.h"

namespace Syndra {

	class Entity {

	public:
		Entity() = default;

		Entity(entt::entity handle, Scene* scene);

		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			SN_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			T& component = m_Scene->m_Registry.emplace<T>(m_EntityID, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			SN_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityID);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.has<T>(m_EntityID);
		}

		template<typename T>
		void RemoveComponent()
		{
			SN_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityID);
		}

		bool operator ==(const Entity& other) const {
			return m_EntityID == other.m_EntityID && m_Scene == other.m_Scene;
		}

		bool operator !=(const Entity& other) {
			return !(*this == other);
		}

		operator bool() const {
			return m_EntityID != entt::null;
		}

		operator entt::entity() const {
			return m_EntityID;
		}

		operator uint32_t() const { 
			return (uint32_t)m_EntityID;
		}

	private:
		entt::entity m_EntityID{ entt::null };
		Scene* m_Scene = nullptr;

	};

}