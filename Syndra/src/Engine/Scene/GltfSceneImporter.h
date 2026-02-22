#pragma once

#include "Engine/Scene/Entity.h"

namespace Syndra {

	class Scene;

	class GltfSceneImporter
	{
	public:
		static bool ImportIntoEntity(Scene& scene, Entity root, const std::string& path);
	};

}

