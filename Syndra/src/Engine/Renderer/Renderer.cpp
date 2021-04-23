#include "lpch.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/Texture.h"

namespace Syndra {

	Renderer::SceneData* Renderer::m_SceneData = new Renderer::SceneData;

	void Renderer::BeginScene(const PerspectiveCamera& camera)
	{
		m_SceneData->ViewProjectionMatrix = camera.GetViewProjection();
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::Submit(const Ref<Shader>& shader,const Ref<VertexArray>& vertexArray)
	{
		shader->Bind();
		shader->SetMat4("u_ViewProjection", m_SceneData->ViewProjectionMatrix);
		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

	void Renderer::Submit(const Ref<Shader>& shader, const Model& model)
	{
		shader->Bind();
		shader->SetMat4("u_ViewProjection", m_SceneData->ViewProjectionMatrix);

		auto& meshes = model.meshes;
		for (auto mesh : meshes) {

			// bind appropriate textures
			unsigned int diffuseNr = 1;
			unsigned int specularNr = 1;
			unsigned int normalNr = 1;
			unsigned int heightNr = 1;
			for (unsigned int i = 0; i < mesh.textures.size(); i++)
			{
				Texture2D::BindTexture(mesh.textures[i].id, i);
				// active proper texture unit before binding
				// retrieve texture number (the N in diffuse_textureN)
				std::string number;
				std::string name = mesh.textures[i].type;
				if (name == "texture_diffuse")
					number = std::to_string(diffuseNr++);
				else if (name == "texture_specular")
					number = std::to_string(specularNr++); // transfer unsigned int to stream
				else if (name == "texture_normal")
					number = std::to_string(normalNr++); // transfer unsigned int to stream
				else if (name == "texture_height")
					number = std::to_string(heightNr++); // transfer unsigned int to stream

				// now set the sampler to the correct texture unit
				//glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
				//// and finally bind the texture
				//glBindTexture(GL_TEXTURE_2D, textures[i].id);
				shader->SetInt(name + number,i);
			}

			auto vertexArray = mesh.GetVertexArray();
			vertexArray->Bind();
			RenderCommand::DrawIndexed(vertexArray);
		}
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	std::string Renderer::RendererInfo()
	{
		return RenderCommand::GetInfo();
	}

}