#include "lpch.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/Instrument.h"
#include "Engine/Renderer/RenderCommand.h"
#include <glad/glad.h>
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
		SN_PROFILE_SCOPE("Renderer::Submit(VertexArray)");
		shader->Bind();
		//shader->SetMat4("u_ViewProjection", m_SceneData->ViewProjectionMatrix);
		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}


	void Renderer::Submit(const Ref<Shader>& shader, const Model& model)
	{
		SN_PROFILE_SCOPE("Renderer::Submit(Model)");
		shader->Bind();
		auto& meshes = model.meshes;
		for (auto& mesh : meshes) {
			Texture2D::BindTexture(0, 0);
			Texture2D::BindTexture(0, 1);
			Texture2D::BindTexture(0, 2);
			Texture2D::BindTexture(0, 3);
			Texture2D::BindTexture(0, 4);
			const auto& meshMaterialData = mesh.GetMaterialData();
			if (meshMaterialData.IsPBR)
			{
				shader->SetFloat4("push.material.color", meshMaterialData.BaseColorFactor);
				shader->SetFloat("push.material.MetallicFactor", meshMaterialData.MetallicFactor);
				shader->SetFloat("push.material.RoughnessFactor", meshMaterialData.RoughnessFactor);
				shader->SetFloat("push.material.AO", meshMaterialData.AOFactor);
				shader->SetFloat("push.tiling", 1.0f);

				const int hasAlbedoMap = meshMaterialData.AlbedoTextureID != 0 ? 1 : 0;
				const int hasMetallicMap = meshMaterialData.MetallicTextureID != 0 ? 1 : 0;
				const int hasNormalMap = meshMaterialData.NormalTextureID != 0 ? 1 : 0;
				const int hasRoughnessMap = meshMaterialData.RoughnessTextureID != 0 ? 1 : 0;
				const int hasAOMap = meshMaterialData.AOTextureID != 0 ? 1 : 0;

				shader->SetInt("push.HasAlbedoMap", hasAlbedoMap);
				shader->SetInt("push.HasMetallicMap", hasMetallicMap);
				shader->SetInt("push.HasNormalMap", hasNormalMap);
				shader->SetInt("push.HasRoughnessMap", hasRoughnessMap);
				shader->SetInt("push.HasAOMap", hasAOMap);

				if (hasAlbedoMap)
					Texture2D::BindTexture(meshMaterialData.AlbedoTextureID, 0);
				if (hasMetallicMap)
					Texture2D::BindTexture(meshMaterialData.MetallicTextureID, 1);
				if (hasNormalMap)
					Texture2D::BindTexture(meshMaterialData.NormalTextureID, 2);
				if (hasRoughnessMap)
					Texture2D::BindTexture(meshMaterialData.RoughnessTextureID, 3);
				if (hasAOMap)
					Texture2D::BindTexture(meshMaterialData.AOTextureID, 4);
			}
			else
			{
				if (mesh.textures.empty()) {
					Texture2D::BindTexture(0, 0);
				}

				for (unsigned int i = 0; i < mesh.textures.size(); i++)
				{
					const std::string& name = mesh.textures[i].type;
					if (name == "texture_diffuse")
						Texture2D::BindTexture(mesh.textures[i].id, 0);
					else if (name == "texture_specular")
						Texture2D::BindTexture(mesh.textures[i].id, 1);
					else if (name == "texture_normal")
						Texture2D::BindTexture(mesh.textures[i].id, 2);
				}
			}

			auto vertexArray = mesh.GetVertexArray();
			vertexArray->Bind();
			RenderCommand::DrawIndexed(vertexArray);
		}
	}

	void Renderer::Submit(Material& material, const Model& model)
	{
		SN_PROFILE_SCOPE("Renderer::Submit(Material,Model)");
		material.Bind();
		auto& meshes = model.meshes;
		for (auto& mesh : meshes) {
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
