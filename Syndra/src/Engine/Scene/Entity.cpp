#include "lpch.h"
#include "Entity.h"

namespace Syndra {

	Scene* Entity::s_Scene = nullptr;

	Entity::Entity(entt::entity handle)
		:m_EntityID(handle)
	{
	}

}
