#pragma once
#include "Engine/Renderer/Shader.h"

namespace Syndra {

	class Material {

	public:

		Material(Ref<Shader>& shader);

		static Ref<Material> Create(Ref<Shader>& shader);

	};

}