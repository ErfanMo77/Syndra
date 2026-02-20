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
		if (name == "push.material.MetallicFactor") {
			m_Cbuffer.material.MetallicFactor = value;
		}
		else if (name == "push.material.RoughnessFactor") {
			m_Cbuffer.material.RoughnessFactor = value;
		}
		else if(name == "push.material.AO")
		{
			m_Cbuffer.material.AO = value;
		}
		else if (name == "tiling") {
			m_Cbuffer.tiling = value;
		}
	}

	void Material::Set(const std::string& name, int value)
	{
		if (name == "HasAlbedoMap") {
			m_Cbuffer.HasAlbedoMap = value;
		}
		else if (name == "HasNormalMap") {
			m_Cbuffer.HasNormalMap = value;
		}
		else if (name == "HasRoughnessMap") {
			m_Cbuffer.HasRoughnessMap = value;
		}
		else if (name == "HasMetallicMap") {
			m_Cbuffer.HasMetallicMap = value;
		}
		else if (name == "HasAOMap") {
			m_Cbuffer.HasAOMap = value;
		}
	}

	void Material::Set(const std::string& name, const glm::vec4& value)
	{
		if (name == "push.material.color") {
			m_Cbuffer.material.color = value;
		}
	}

	void Material::Set(const std::string& name, const glm::vec3& value)
	{

	}

	void Material::SetSamplersUsed()
	{
		for (auto& sampler : m_Samplers) {
			if (sampler.binding == 0)
				sampler.isUsed = m_Cbuffer.HasAlbedoMap;
			if (sampler.binding == 1)
				sampler.isUsed = m_Cbuffer.HasMetallicMap;
			if (sampler.binding == 2)
				sampler.isUsed = m_Cbuffer.HasNormalMap;
			if (sampler.binding == 3)
				sampler.isUsed = m_Cbuffer.HasRoughnessMap;
			if (sampler.binding == 4)
				sampler.isUsed = m_Cbuffer.HasAOMap;
		}
	}

	Ref<Material> Material::Create(Ref<Shader>& shader)
	{
		return CreateRef<Material>(shader);
	}

	void Material::Bind()
	{
		m_Shader->Bind();

		//Binding textures
		for (auto& sampler : m_Samplers)
		{
			auto& texture = m_Textures[sampler.binding];
			if (sampler.isUsed && texture) {

				if (sampler.binding == 0)
					m_Shader->SetInt("push.HasAlbedoMap", 1);
				if (sampler.binding == 1)
					m_Shader->SetInt("push.HasMetallicMap", 1);
				if (sampler.binding == 2)
					m_Shader->SetInt("push.HasNormalMap", 1);
				if (sampler.binding == 3)
					m_Shader->SetInt("push.HasRoughnessMap", 1);
				if (sampler.binding == 4)
					m_Shader->SetInt("push.HasAOMap", 1);
				texture->Bind(sampler.binding);
			}
			else
			{
				Texture2D::BindTexture(0, sampler.binding);
				if (sampler.binding == 0)
					m_Shader->SetInt("push.HasAlbedoMap", 0);
				if (sampler.binding == 1)
					m_Shader->SetInt("push.HasMetallicMap", 0);
				if (sampler.binding == 2)
					m_Shader->SetInt("push.HasNormalMap", 0);
				if (sampler.binding == 3)
					m_Shader->SetInt("push.HasRoughnessMap", 0);
				if (sampler.binding == 4)
					m_Shader->SetInt("push.HasAOMap", 0);
			}
		}

		bool hasMaterialConstants = false;
		for (const auto& pushConstant : m_PushConstants)
		{
			for (const auto& item : pushConstant.members)
			{
				if (item.name == "material" ||
					item.name == "color" ||
					item.name == "RoughnessFactor" ||
					item.name == "MetallicFactor" ||
					item.name == "AO")
				{
					hasMaterialConstants = true;
					break;
				}
			}

			if (hasMaterialConstants)
				break;
		}

		if (hasMaterialConstants || m_PushConstants.empty())
		{
			m_Shader->SetFloat("push.material.MetallicFactor", m_Cbuffer.material.MetallicFactor);
			m_Shader->SetFloat("push.material.RoughnessFactor", m_Cbuffer.material.RoughnessFactor);
			m_Shader->SetFloat("push.material.AO", m_Cbuffer.material.AO);
			m_Shader->SetFloat4("push.material.color", m_Cbuffer.material.color);
			m_Shader->SetFloat("push.tiling", m_Cbuffer.tiling);
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
