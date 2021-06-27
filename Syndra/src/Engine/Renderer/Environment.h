#pragma once
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/FrameBuffer.h"


namespace Syndra {

	class Environment {

	public:
		Environment(const Ref<Texture2D>& hdri);
	
		void RenderBackground();
		void SetHDRTexture(const Ref<Texture2D>& hdri) { m_HDRSkyMap = hdri; }
		void SetViewProjection(const glm::mat4& view, const glm::mat4& projection) { m_View = view; m_Projection = projection; }

		void SetIntensity(float intensity);

		uint32_t GetBackgroundTextureID() const;
		std::string GetPath() { return m_HDRSkyMap->GetPath(); }

		void BindIrradianceMap(uint32_t slot);
		void BindPreFilterMap(uint32_t slot);
		void BindBRDFMap(uint32_t slot);

	private:
		void SetupCube();
		void SetupFrameBuffer();

		void RenderCube();
		void RenderQuad();

	private:
		unsigned int envCubemap;
		unsigned int brdfLUTTexture;
		unsigned int prefilterMap;
		unsigned int irradianceMap;
		Ref<Texture2D> m_HDRSkyMap;
		Ref<Shader> m_EquirectangularToCube, m_BackgroundShader, m_IrradianceConvShader, m_PrefilterShader, m_BRDFLutShader;
		Ref<VertexArray> m_CubeVAO, m_QuadVAO;
		Ref<FrameBuffer> m_IrradianceFBO, m_PrefilterFBO;
		glm::mat4 m_View, m_Projection;
	};

}