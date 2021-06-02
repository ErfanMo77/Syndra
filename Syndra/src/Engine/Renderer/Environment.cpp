#include "lpch.h"
#include "Engine/Renderer/Environment.h"
#include "glad/glad.h"

namespace Syndra {

	Environment::Environment(const Ref<Texture2D>& hdri)
		:m_HDRSkyMap(hdri)
	{
		m_EquirectangularToCube = Shader::Create("assets/shaders/EquirectangularToCube.glsl");
		m_BackgroundShader = Shader::Create("assets/shaders/BackgroundSky.glsl");
		m_BackgroundShader->Bind();
		m_BackgroundShader->SetFloat("push.intensity", 0.5f);
		m_BackgroundShader->Unbind();
		m_IrradianceConvShader = Shader::Create("assets/shaders/IrradianceConvolution.glsl");
		SetupCube();
		SetupFrameBuffer();
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};
		
		m_CaptureFBO->Bind();

		//Converting hdr sky to cube map
		m_EquirectangularToCube->Bind();
		m_EquirectangularToCube->SetMat4("cam.projection", captureProjection);

		m_HDRSkyMap->Bind(0);
		for (unsigned int i = 0; i < 6; ++i)
		{
			m_EquirectangularToCube->SetMat4("cam.view", captureViews[i]);
			m_CaptureFBO->BindCubemapFace(i);
			RenderCommand::Clear();
			RenderCube();
		}
		m_EquirectangularToCube->Unbind();
		m_CaptureFBO->Unbind();

		// cube map Convolution
		m_IrradianceFBO->Bind();
		m_IrradianceConvShader->Bind();
		m_IrradianceConvShader->SetMat4("cam.projection", captureProjection);
		Texture2D::BindTexture(m_CaptureFBO->GetColorAttachmentRendererID(), 0);
		
		for (unsigned int i = 0; i < 6; ++i)
		{
			m_IrradianceConvShader->SetMat4("cam.view", captureViews[i]);
			m_IrradianceFBO->BindCubemapFace(i);
			RenderCommand::Clear();
			RenderCube();
		}
		m_IrradianceConvShader->Unbind();
		m_IrradianceFBO->Unbind();
		
	}

	void Environment::RenderCube()
	{
		m_CubeVAO->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	void Environment::RenderBackground()
	{
		m_BackgroundShader->Bind();
		Texture2D::BindTexture(m_CaptureFBO->GetColorAttachmentRendererID(), 0);
		//Texture2D::BindTexture(m_IrradianceFBO->GetColorAttachmentRendererID(), 1);
		m_BackgroundShader->SetMat4("cam.view", m_View);
		m_BackgroundShader->SetMat4("cam.projection", m_Projection);

		RenderCube();
	}

	void Environment::SetIntensity(float intensity)
	{
		m_BackgroundShader->Bind();
		m_BackgroundShader->SetFloat("push.intensity", intensity);
		m_BackgroundShader->Unbind();
	}

	void Environment::BindIrradianceMap(uint32_t slot)
	{
		Texture2D::BindTexture(m_IrradianceFBO->GetColorAttachmentRendererID(), slot);
	}

	void Environment::SetupCube()
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		m_CubeVAO = VertexArray::Create();
		Ref<VertexBuffer> cubeVBO = VertexBuffer::Create(vertices, sizeof(vertices));
		BufferLayout cubeLayout = { 
			{ShaderDataType::Float3, "a_pos" } ,
			{ShaderDataType::Float3, "a_normal" } ,
			{ShaderDataType::Float2, "a_uv" }
		
		};
		cubeVBO->SetLayout(cubeLayout);
		m_CubeVAO->AddVertexBuffer(cubeVBO);
	}

	void Environment::SetupFrameBuffer()
	{
		FramebufferSpecification spec;
		spec.Attachments = { FramebufferTextureFormat::Cubemap ,FramebufferTextureFormat::Depth };
		spec.Width = 2048;
		spec.Height = 2048;
		spec.Samples = 1;
		spec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		m_CaptureFBO = FrameBuffer::Create(spec);

		spec.Width = 32;
		spec.Height = 32;
		m_IrradianceFBO = FrameBuffer::Create(spec);
	}

}