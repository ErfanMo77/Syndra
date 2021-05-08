#pragma once
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Texture.h"

namespace Syndra {

	struct MaterialTexture
	{
		Ref<Texture2D> tex;
		uint32_t slot;
		bool isUsed;
	};

	class Material {

	public:

		Material() = default;
		Material(Ref<Shader>& shader);
		void Bind();

		std::vector<MaterialTexture>& GetTextures() { return m_Textures; }

		std::vector<PushConstant>& GetPushConstants() { return m_PushConstants; }
		std::vector<Sampler>& GetSamplers() { return m_Samplers; }

		static Ref<Material> Create(Ref<Shader>& shader);

	private:
		Ref<Shader> m_Shader;

		std::vector<MaterialTexture> m_Textures;

		std::vector<PushConstant> m_PushConstants;
		std::vector<Sampler> m_Samplers;

	};

}