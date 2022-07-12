#include "lpch.h"
#include "DeferredRenderer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Core/Application.h"

#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"
#include <glad/glad.h>

namespace Syndra {
	
	static DeferredRenderer::RenderData r_Data;
	
	void DeferredRenderer::Init(const Ref<Scene>& scene, const ShaderLibrary& shaders, const Ref<Environment>& env)
	{
		r_Data.scene = scene;
		r_Data.shaders = shaders;
		r_Data.environment = env;
		
		//------------------------------------Deferred Geometry Render Pass--------------------------------------//
		FramebufferSpecification GeoFbSpec;
		GeoFbSpec.Attachments =
		{
			FramebufferTextureFormat::RGBA16F,			// Position texture attachment
			FramebufferTextureFormat::RGBA16F,			// Normal texture attachment
			FramebufferTextureFormat::RGBA16F,			// Albedo texture attachment
			FramebufferTextureFormat::RGBA16F,		    // Roughness-Metallic-AO texture attachment
			FramebufferTextureFormat::RED_INTEGER,		// Entities ID texture attachment
			FramebufferTextureFormat::DEPTH24STENCIL8	// default depth map
		};
		GeoFbSpec.Width = 1280;
		GeoFbSpec.Height = 720;
		GeoFbSpec.Samples = 1;
		GeoFbSpec.ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		RenderPassSpecification GeoPassSpec;
		GeoPassSpec.TargetFrameBuffer = FrameBuffer::Create(GeoFbSpec);
		r_Data.geoPass = RenderPass::Create(GeoPassSpec);

		//-------------------------------------------Lighting and Post Processing Pass---------------------------//
		FramebufferSpecification postProcFB;
		postProcFB.Attachments = { FramebufferTextureFormat::RGBA8 , FramebufferTextureFormat::DEPTH24STENCIL8 };
		postProcFB.Width = 1280;
		postProcFB.Height = 720;
		postProcFB.Samples = 1;
		postProcFB.ClearColor = glm::vec4(0.196f, 0.196f, 0.196f, 1.0f);

		RenderPassSpecification finalPassSpec;
		finalPassSpec.TargetFrameBuffer = FrameBuffer::Create(postProcFB);
		r_Data.lightingPass = RenderPass::Create(finalPassSpec);

		//-----------------------------------------------Shadow Pass---------------------------------------------//

		//Directional Light shadow map
		FramebufferSpecification shadowSpec;
		shadowSpec.Attachments = { FramebufferTextureFormat::DEPTH32 };
		shadowSpec.Width = 4096;
		shadowSpec.Height = 4096;
		shadowSpec.Samples = 1;
		shadowSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		RenderPassSpecification shadowPassSpec;
		shadowPassSpec.TargetFrameBuffer = FrameBuffer::Create(shadowSpec);
		r_Data.shadowPass = RenderPass::Create(shadowPassSpec);

		//-----------------------------------------------Anti Aliasing------------------------------------------//
		FramebufferSpecification aaFB;
		aaFB.Attachments = { FramebufferTextureFormat::RGBA8 };
		aaFB.Width = 1280;
		aaFB.Height = 720;
		aaFB.Samples = 1;
		aaFB.ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		RenderPassSpecification aaPassSpec;
		aaPassSpec.TargetFrameBuffer = FrameBuffer::Create(aaFB);
		r_Data.aaPass = RenderPass::Create(aaPassSpec);

		//------------------------------------------------Deferred Shaders--------------------------------------//
		r_Data.depth = r_Data.shaders.Get("depth");
		r_Data.geoShader = r_Data.shaders.Get("GeometryPass");
		r_Data.fxaa = r_Data.shaders.Get("FXAA");
		r_Data.deferredLighting = r_Data.shaders.Get("DeferredLighting");
		
		//----------------------------------------------SCREEN QUAD---------------------------------------------//
		r_Data.screenVao = VertexArray::Create();
		float quad[] = {
			 1.0f,  1.0f, 0.0f,    1.0f, 1.0f,   // top right
			 1.0f, -1.0f, 0.0f,    1.0f, 0.0f,   // bottom right
			-1.0f, -1.0f, 0.0f,    0.0f, 0.0f,   // bottom left
			-1.0f,  1.0f, 0.0f,    0.0f, 1.0f    // top left 
		};
		auto vb = VertexBuffer::Create(quad, sizeof(quad));
		BufferLayout quadLayout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float2,"a_uv"},
		};
		vb->SetLayout(quadLayout);
		r_Data.screenVao->AddVertexBuffer(vb);
		unsigned int quadIndices[] = {
			0, 3, 1, // first triangle
			1, 3, 2  // second triangle
		};
		auto eb = IndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t));
		r_Data.screenVao->SetIndexBuffer(eb);
		
		// lighting parameters
		r_Data.exposure = 0.5f;
		r_Data.gamma = 1.9f;
		r_Data.lightSize = 1.0f;
		r_Data.orthoSize = 20.0f;
		r_Data.lightNear = 20.0f;
		r_Data.lightFar = 200.0f;
		r_Data.intensity = 1.0f;

		Syndra::Math::GeneratePoissonDisk(r_Data.distributionSampler0, 64);
		Syndra::Math::GeneratePoissonDisk(r_Data.distributionSampler1, 64);

		r_Data.deferredLighting->Bind();
		r_Data.deferredLighting->SetFloat("pc.near", r_Data.lightNear);

		//Light uniform Buffer layout: -- point lights -- spotlights -- directional light--Binding point 2
		r_Data.lightManager = CreateRef<LightManager>(2);

		float dSize = r_Data.orthoSize;
		r_Data.lightProj = glm::ortho(-dSize, dSize, -dSize, dSize, r_Data.lightNear, r_Data.lightFar);
		//r_Data.lightProj = glm::perspective(45.0f, 1.0f, r_Data.lightNear, r_Data.lightFar);
		r_Data.ShadowBuffer = UniformBuffer::Create(sizeof(glm::mat4) * 25, 3);
		r_Data.lightManager->IntitializeLights();
	}

	void DeferredRenderer::Render()
	{
		auto view = r_Data.scene->m_Registry.view<TransformComponent, MeshComponent>();
		//---------------------------------------------------------SHADOW PASS------------------------------------------//
		r_Data.shadowPass->BindTargetFrameBuffer();
		RenderCommand::SetState(RenderState::DEPTH_TEST, true);
		RenderCommand::SetClearColor(r_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
		r_Data.depth->Bind();
		RenderCommand::Clear();
		for (auto ent : view)
		{
			auto& tc = view.get<TransformComponent>(ent);
			auto& mc = view.get<MeshComponent>(ent);
			if (!mc.path.empty())
			{
				r_Data.depth->SetMat4("transform.u_trans", tc.GetTransform());
				Renderer::Submit(r_Data.depth, mc.model);
			}
		}
		r_Data.shadowPass->UnbindTargetFrameBuffer();

		//--------------------------------------------------GEOMETRY PASS----------------------------------------------//
		r_Data.geoPass->BindTargetFrameBuffer();
		RenderCommand::SetState(RenderState::DEPTH_TEST, true);
		RenderCommand::SetClearColor(r_Data.geoPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
		r_Data.geoPass->GetSpecification().TargetFrameBuffer->ClearAttachment(4, -1);
		r_Data.geoShader->Bind();
		RenderCommand::Clear();
		for (auto ent : view)
		{
			auto& tc = view.get<TransformComponent>(ent);
			auto& mc = view.get<MeshComponent>(ent);
			if (!mc.path.empty())
			{
				if (r_Data.scene->m_Registry.has<MaterialComponent>(ent)) {
					auto& mat = r_Data.scene->m_Registry.get<MaterialComponent>(ent);
					r_Data.geoShader->SetInt("transform.id", (uint32_t)ent);
					r_Data.geoShader->SetMat4("transform.u_trans", tc.GetTransform());
					Renderer::Submit(mat.m_Material, mc.model);
				}
				else
				{
					r_Data.geoShader->SetInt("push.HasAlbedoMap", 1);
					r_Data.geoShader->SetFloat("push.tiling", 1.0f);
					r_Data.geoShader->SetInt("push.HasNormalMap", 0);
					r_Data.geoShader->SetInt("push.HasMetallicMap", 0);
					r_Data.geoShader->SetInt("push.HasRoughnessMap", 0);
					r_Data.geoShader->SetInt("push.HasAOMap", 0);
					r_Data.geoShader->SetFloat("push.material.MetallicFactor", 0);
					r_Data.geoShader->SetFloat("push.material.RoughnessFactor", 1);
					r_Data.geoShader->SetFloat("push.material.AO", 1);
					r_Data.geoShader->SetMat4("transform.u_trans", tc.GetTransform());
					r_Data.geoShader->SetInt("transform.id", (uint32_t)ent);
					Renderer::Submit(r_Data.geoShader, mc.model);
				}
			}
		}
		r_Data.geoShader->Unbind();
		r_Data.geoPass->UnbindTargetFrameBuffer();
	}

	void DeferredRenderer::End()
	{
		//-------------------------------------------------Lighting and post-processing pass---------------------------------------------------//

		r_Data.lightingPass->BindTargetFrameBuffer();

		RenderCommand::SetState(RenderState::DEPTH_TEST, false);
		r_Data.screenVao->Bind();

		r_Data.deferredLighting->Bind();
		//shadow map samplers
		Texture2D::BindTexture(r_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 3);
		Texture1D::BindTexture(r_Data.distributionSampler0->GetRendererID(), 4);
		Texture1D::BindTexture(r_Data.distributionSampler1->GetRendererID(), 5);

		//Push constant variables
		r_Data.deferredLighting->SetFloat("pc.size", r_Data.lightSize * 0.0001f);
		r_Data.deferredLighting->SetInt("pc.numPCFSamples", r_Data.numPCF);
		r_Data.deferredLighting->SetInt("pc.numBlockerSearchSamples", r_Data.numBlocker);
		r_Data.deferredLighting->SetInt("pc.softShadow", (int)r_Data.softShadow);
		r_Data.deferredLighting->SetFloat("pc.exposure", r_Data.exposure);
		r_Data.deferredLighting->SetFloat("pc.gamma", r_Data.gamma);
		r_Data.deferredLighting->SetFloat("pc.near", r_Data.lightNear);
		r_Data.deferredLighting->SetFloat("pc.intensity", r_Data.intensity);
		//GBuffer samplers
		Texture2D::BindTexture(r_Data.geoPass->GetFrameBufferTextureID(0), 0);
		Texture2D::BindTexture(r_Data.geoPass->GetFrameBufferTextureID(1), 1);
		Texture2D::BindTexture(r_Data.geoPass->GetFrameBufferTextureID(2), 2);
		Texture2D::BindTexture(r_Data.geoPass->GetFrameBufferTextureID(3), 6);
		if (r_Data.environment) {
			r_Data.environment->SetIntensity(r_Data.intensity);
			r_Data.environment->BindIrradianceMap(7);
			r_Data.environment->BindPreFilterMap(8);
			r_Data.environment->BindBRDFMap(9);
		}
		Renderer::Submit(r_Data.deferredLighting, r_Data.screenVao);

		r_Data.deferredLighting->Unbind();

		r_Data.lightingPass->BindTargetFrameBuffer();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, r_Data.geoPass->GetSpecification().TargetFrameBuffer->GetRendererID());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetRendererID()); // write to default framebuffer
		auto w = r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width;
		auto h = r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height;
		glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		if (r_Data.environment) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			r_Data.environment->RenderBackground();
			glDepthFunc(GL_LESS);
		}

		r_Data.lightingPass->UnbindTargetFrameBuffer();
		//-------------------------------------------------ANTI ALIASING PASS------------------------------------------------------------------//

		if (r_Data.useFxaa) {
			r_Data.aaPass->BindTargetFrameBuffer();
			r_Data.fxaa->Bind();
			r_Data.fxaa->SetFloat("pc.width", (float)r_Data.aaPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width);
			r_Data.fxaa->SetFloat("pc.height", (float)r_Data.aaPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height);
			Texture2D::BindTexture(r_Data.lightingPass->GetFrameBufferTextureID(0), 0);
			Renderer::Submit(r_Data.fxaa, r_Data.screenVao);
			r_Data.fxaa->Unbind();
			r_Data.aaPass->UnbindTargetFrameBuffer();
		}

		Renderer::EndScene();
	}

	void DeferredRenderer::ShutDown()
	{
	}

	void DeferredRenderer::OnImGuiRender(bool* rendererOpen, bool* environmentOpen)
	{
		//Renderer settings
		if (*rendererOpen) {
			ImGui::Begin(ICON_FA_COGS" Renderer Settings", rendererOpen);

			ImGui::Text("Geometry pass debugger");
			static bool showAlbedo = false;
			static bool showNormal = false;
			static bool showPosition = false;
			static bool showRoughMetalAO = false;
			if (ImGui::Button("Albedo")) {
				showAlbedo = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Normal")) {
				showNormal = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Position")) {
				showPosition = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("RoughMetalAO")) {
				showRoughMetalAO = true;
			}
			auto width = r_Data.geoPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width * 0.5f;
			auto height = r_Data.geoPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height * 0.5f;
			auto ratio = height / width;
			width = ImGui::GetContentRegionAvail().x;
			height = ratio * width;
			if (showAlbedo) {
				ImGui::Begin("Albedo", &showAlbedo);
				width = ImGui::GetContentRegionAvail().x;
				height = ratio * width;
				ImGui::Image((ImTextureID)r_Data.geoPass->GetFrameBufferTextureID(2), { width, height }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				ImGui::End();
			}

			if (showNormal) {
				ImGui::Begin("Normal", &showNormal);
				width = ImGui::GetContentRegionAvail().x;
				height = ratio * width;
				ImGui::Image((ImTextureID)r_Data.geoPass->GetFrameBufferTextureID(1), { width, height }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				ImGui::End();
			}

			if (showPosition) {
				ImGui::Begin("Position", &showPosition);
				width = ImGui::GetContentRegionAvail().x;
				height = ratio * width;
				ImGui::Image((ImTextureID)r_Data.geoPass->GetFrameBufferTextureID(0), { width, height }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				ImGui::End();
			}

			if (showRoughMetalAO) {
				ImGui::Begin("RoughMetalAO", &showRoughMetalAO);
				width = ImGui::GetContentRegionAvail().x;
				height = ratio * width;
				ImGui::Image((ImTextureID)r_Data.geoPass->GetFrameBufferTextureID(3), { width, height }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				ImGui::End();
			}
			ImGui::Separator();
			//V-Sync
			static bool vSync = true;
			ImGui::Checkbox("V-Sync", &vSync);
			Application::Get().GetWindow().SetVSync(vSync);
			ImGui::Separator();

			ImGui::Text("Anti Aliasing");
			ImGui::Checkbox("FXAA", &r_Data.useFxaa);
			ImGui::Separator();

			//Exposure
			ImGui::DragFloat("exposure", &r_Data.exposure, 0.01f, -2, 4);

			//Gamma
			ImGui::DragFloat("gamma", &r_Data.gamma, 0.01f, 0, 4);

			ImGui::Checkbox("Soft Shadow", &r_Data.softShadow);
			ImGui::DragFloat("PCF samples", &r_Data.numPCF, 1, 1, 64);
			ImGui::DragFloat("blocker samples", &r_Data.numBlocker, 1, 1, 64);


			//shadow
			ImGui::DragFloat("Light Size", &r_Data.lightSize, 0.01f, 0, 100);

			if (ImGui::DragFloat("Ortho Size", &r_Data.orthoSize, 0.1f, 1, 100)) {
				r_Data.lightProj = glm::ortho(-r_Data.orthoSize, r_Data.orthoSize, -r_Data.orthoSize, r_Data.orthoSize, r_Data.lightNear, r_Data.lightFar);
			}

			ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
			if (ImGui::DragFloat("near", &r_Data.lightNear, 0.01f, 0.1f, 100.0f)) {
				//r_Data.lightProj = glm::perspective(45.0f, 1.0f, r_Data.lightNear, r_Data.lightFar);
				r_Data.lightProj = glm::ortho(-r_Data.orthoSize, r_Data.orthoSize, -r_Data.orthoSize, r_Data.orthoSize, r_Data.lightNear, r_Data.lightFar);
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			if (ImGui::DragFloat("far", &r_Data.lightFar, 0.1f, 100.0f, 10000.0f)) {
				//r_Data.lightProj = glm::perspective(45.0f, 1.0f, r_Data.lightNear, r_Data.lightFar);
				r_Data.lightProj = glm::ortho(-r_Data.orthoSize, r_Data.orthoSize, -r_Data.orthoSize, r_Data.orthoSize, r_Data.lightNear, r_Data.lightFar);
			}
			ImGui::PopItemWidth();

			ImGui::Separator();

			ImGui::End();
		}

		//Environment settings
		if (*environmentOpen) {
			ImGui::Begin(ICON_FA_TREE" Environment", environmentOpen);
			if (ImGui::Button("HDR", { 40,30 })) {
				auto path = FileDialogs::OpenFile("HDR (*.hdr)\0*.hdr\0");
				if (path) {
					//Add texture as sRGB color space if it is binded to 0 (diffuse texture binding)
					r_Data.environment = CreateRef<Environment>(Texture2D::CreateHDR(*path, false, true));
					r_Data.scene->m_EnvironmentPath = *path;
					SceneRenderer::SetEnvironment(r_Data.environment);
				}
			}
			if (r_Data.environment) {
				ImGui::Image((ImTextureID)r_Data.environment->GetBackgroundTextureID(), { 300, 150 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			}
			if (ImGui::DragFloat("Intensity", &r_Data.intensity, 0.01f, 1, 20)) {
				if (r_Data.environment) {
					r_Data.environment->SetIntensity(r_Data.intensity);
				}
			}
			ImGui::End();
		}
	}

	void DeferredRenderer::OnResize(uint32_t width, uint32_t height)
	{
		r_Data.geoPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		r_Data.lightingPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		r_Data.aaPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
	}

	void DeferredRenderer::UpdateLights()
	{
		r_Data.lightManager->IntitializeLights();
		auto viewLights = r_Data.scene->m_Registry.view<TransformComponent, LightComponent>();
		//point light index
		int pIndex = 0;
		//spot light index
		int sIndex = 0;
		//Set light values for each entity that has a light component
		for (auto ent : viewLights)
		{
			auto& tc = viewLights.get<TransformComponent>(ent);
			auto& lc = viewLights.get<LightComponent>(ent);

			if (lc.type == LightType::Directional) {
				auto p = dynamic_cast<DirectionalLight*>(lc.light.get());
				r_Data.lightManager->UpdateDirLight(p, tc.Translation);
				//shadow
				r_Data.lightView = glm::lookAt(-(glm::normalize(p->GetDirection()) * r_Data.lightFar / 4.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				r_Data.shadowData.lightViewProj = r_Data.lightProj * r_Data.lightView;
				r_Data.ShadowBuffer->SetData(&r_Data.shadowData, sizeof(glm::mat4));
				p = nullptr;
			}
			if (lc.type == LightType::Point) {
				if (pIndex < 4) {
					auto p = dynamic_cast<PointLight*>(lc.light.get());
					r_Data.lightManager->UpdatePointLights(p, tc.Translation, pIndex);
					pIndex++;
					p = nullptr;
				}
			}
			if (lc.type == LightType::Spot) {
				if (sIndex < 4) {
					auto p = dynamic_cast<SpotLight*>(lc.light.get());
					r_Data.lightManager->UpdateSpotLights(p, tc.Translation, sIndex);
					sIndex++;
					p = nullptr;
				}
			}
		}
		//Filling light buffer data with different light values
		r_Data.lightManager->UpdateBuffer();
	}

	uint32_t DeferredRenderer::GetFinalTextureID(int index)
	{
		if (r_Data.useFxaa) {
			return r_Data.aaPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(index);
		}
		else
		{
			return r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(index);
		}
	}

	uint32_t DeferredRenderer::GetMouseTextureID()
	{
		return 4;
	}

	Ref<FrameBuffer> DeferredRenderer::GetMainFrameBuffer()
	{
		return r_Data.geoPass->GetSpecification().TargetFrameBuffer;
	}

}
