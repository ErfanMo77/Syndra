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
		//for (auto& texture : m_Textures)
		//{
		//	if (*texture.isUsed)
		//	{
		//		texture.tex->Bind(texture.slot);
		//	}
		//}

		for (auto& sampler : m_Samplers)
		{
			if (sampler.isUsed) {
				auto& texture = m_Textures[sampler.binding];
				if (texture) {
					texture->Bind(sampler.binding);
				}
			}
		}

		//Binding push constants

	}

	void Material::AddTexture(const Sampler& sampler, Ref<Texture2D>& texture)
	{
		m_Textures.insert(std::pair(sampler.binding, texture));
	}

	//MaterialTexture Material::AddTexture(MaterialTexture& mt)
	//{
	//	m_Textures.push_back(mt);
	//	return mt;
	//}

	Ref<Texture2D> Material::GetTexture(const Sampler& sampler)
	{
		return  m_Textures[sampler.binding];
	}


	//bool Material::IsBinded(const Sampler& sampler) const
	//{
	//	for (auto& texture : m_Textures)
	//	{
	//		if (texture.slot == sampler.binding) {
	//			return true;
	//		}
	//	}
	//	return false;
	//}

}
