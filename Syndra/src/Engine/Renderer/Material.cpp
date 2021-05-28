#include "lpch.h"
#include "Engine/Renderer/Material.h"


namespace Syndra {

	Material::Material(Ref<Shader>& shader)
	{
		m_Shader = shader;
		m_Samplers = shader->GetSamplers();
		m_PushConstants = shader->GetPushConstants();
	}

	void Material::Set(const std::string& name, float value)
	{

	}

	void Material::Set(const std::string& name, int value)
	{
		for (auto& item : m_PushConstants[0].members) {
			if (name == item.name) {
				
			}
		}
	}

	void Material::Set(const std::string& name, const glm::vec4& value)
	{

	}

	void Material::Set(const std::string& name, const glm::vec3& value)
	{

	}

	Ref<Material> Material::Create(Ref<Shader>& shader)
	{
		return CreateRef<Material>(shader);
	}

	void Material::Bind()
	{
		m_Shader->Bind();
		
		for (auto& sampler : m_Samplers)
		{
			if (sampler.isUsed) {
				auto& texture = m_Textures[sampler.binding];
				if (texture) {
					if (sampler.binding == 0) {
						m_Shader->SetInt("push.HasDiffuseMap", 1);
					}
					if (sampler.binding == 2)
					{
						m_Shader->SetInt("push.HasNormalMap", 1);
					}
					texture->Bind(sampler.binding);
				}
			}
			else
			{
				if (sampler.binding == 0) {
					m_Shader->SetInt("push.HasDiffuseMap", 0);
				}
				if (sampler.binding == 2)
				{
					m_Shader->SetInt("push.HasNormalMap", 0);
				}
			}
		}

		//Binding push constants
		for (auto& item : m_PushConstants[0].members) 
		{
			
		}
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
