#include "lpch.h"
#include "Engine/Renderer/Material.h"


namespace Syndra {

	Material::Material(Ref<Shader>& shader)
	{
		m_Shader = shader;
		m_Samplers = shader->GetSamplers();
		m_PushConstants = shader->GetPushConstants();
	}

	Ref<Material> Material::Create(Ref<Shader>& shader)
	{
		return CreateRef<Material>(shader);
	}

	void Material::Bind()
	{
		m_Shader->Bind();
		
		//Binding textures
		for (auto& texture : m_Textures)
		{
			if (texture.isUsed)
			{
				texture.tex->Bind(texture.slot);
			}
		}

		//Binding push constants

	}

}
