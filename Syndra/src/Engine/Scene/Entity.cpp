#include "lpch.h"
#include "Entity.h"

namespace Syndra {

	Entity::Entity(entt::entity handle, Scene* scene)
		:m_EntityID(handle),m_Scene(scene)
	{
	}

}
