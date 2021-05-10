#pragma once
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Texture.h"

namespace Syndra {


	class Material {

	public:

		Material() = default;
		Material(const Material& material) = default;
		Material(Ref<Shader>& shader);

		void Bind();

		std::vector<PushConstant>& GetPushConstants() { return m_PushConstants; }
		std::vector<Sampler>& GetSamplers() { return m_Samplers; }
		std::unordered_map<uint32_t, Ref<Texture2D>>& GetTextures() { return m_Textures; }
		
		void AddTexture(const Sampler& sampler, Ref<Texture2D>& texture);


		Ref<Texture2D> GetTexture(const Sampler& sampler);

		bool IsBinded(const Sampler& sampler) const;



		static Ref<Material> Create(Ref<Shader>& shader);

	private:
		Ref<Shader> m_Shader;

		std::unordered_map<uint32_t, Ref<Texture2D>> m_Textures;

		std::vector<PushConstant> m_PushConstants;
		std::vector<Sampler> m_Samplers;

	};

}