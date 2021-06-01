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

	private:
		void SetupCube();
		void SetupFrameBuffer();

		void RenderCube();

	private:
		Ref<Texture2D> m_HDRSkyMap;
		Ref<Shader> m_EquirectangularToCube, m_BackgroundShader;
		Ref<VertexArray> m_CubeVAO;
		Ref<FrameBuffer> m_CaptureFBO;
		glm::mat4 m_View, m_Projection;
	};

}