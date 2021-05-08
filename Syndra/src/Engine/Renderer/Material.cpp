#include "lpch.h"
#include "Engine/Renderer/Material.h"

namespace Syndra {

	Material::Material(Ref<Shader>& shader)
	{
		
	}

	Ref<Material> Material::Create(Ref<Shader>& shader)
	{
		return CreateRef<Material>(shader);
	}

}
