#include "lpch.h"
#include "ForwardPlusRenderer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Core/Application.h"

#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"
#include <glad/glad.h>

namespace Syndra {
	
	static ForwardPlusRenderer::RenderData r_Data;

	void ForwardPlusRenderer::Init(const Ref<Scene>& scene, const ShaderLibrary& shaders, const Ref<Environment>& env)
	{
		r_Data.scene = scene;
		r_Data.shaders = shaders;
		r_Data.environment = env;

		//-------------------------------Depth Pre Pass Framebuffer Initialization--------------------------------//
		FramebufferSpecification depthPassFB;
		depthPassFB.Attachments = { FramebufferTextureFormat::DEPTH32 };
		depthPassFB.Width = 1280;
		depthPassFB.Height = 720;
		depthPassFB.Samples = 1;
		depthPassFB.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		RenderPassSpecification depthPassSpec;
		depthPassSpec.TargetFrameBuffer = FrameBuffer::Create(depthPassFB);
		r_Data.depthPass = RenderPass::Create(depthPassSpec);

		//----------------------------------Shadow Pass Framebuffer Initialization--------------------------------//
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

		//-------------------------------------Lighting and Post Processing Pass----------------------------------//
		FramebufferSpecification lightingFB;
		lightingFB.Attachments = { 
			FramebufferTextureFormat::RGBA8, 
			FramebufferTextureFormat::RED_INTEGER,
			FramebufferTextureFormat::DEPTH24STENCIL8
		};
		lightingFB.Width = 1280;
		lightingFB.Height = 720;
		lightingFB.Samples = 1;
		lightingFB.ClearColor = glm::vec4(0.196f, 0.196f, 0.196f, 1.0f);

		RenderPassSpecification finalPassSpec;
		finalPassSpec.TargetFrameBuffer = FrameBuffer::Create(lightingFB);
		r_Data.lightingPass = RenderPass::Create(finalPassSpec);

		//--------------------------------Anti Aliasing Framebuffer Initialization--------------------------------//
		FramebufferSpecification postProcFB;
		postProcFB.Attachments = { 
			//main texture
			FramebufferTextureFormat::RGBA8,
			//debth debug
			FramebufferTextureFormat::RGBA8
		};
		postProcFB.Width = 1280;
		postProcFB.Height = 720;
		postProcFB.Samples = 1;
		postProcFB.ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		RenderPassSpecification postPassSpec;
		postPassSpec.TargetFrameBuffer = FrameBuffer::Create(postProcFB);
		r_Data.postProcPass = RenderPass::Create(postPassSpec);

		//-----------------------------------------------SCREEN QUAD-----------------------------------------------//
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

		//------------------------------------------------ForwardPlus Shaders--------------------------------------//
		r_Data.depthShader = r_Data.shaders.Get("depthPass");
		r_Data.shadowDepthShader = r_Data.shaders.Get("depth");
		r_Data.postProcShader = r_Data.shaders.Get("ForwardPostProc");
		r_Data.compShader = r_Data.shaders.Get("computeShader");
		r_Data.forwardLightingShader = r_Data.shaders.Get("ForwardShading");


		Syndra::Math::GeneratePoissonDisk(r_Data.distributionSampler0, 64);
		Syndra::Math::GeneratePoissonDisk(r_Data.distributionSampler1, 64);

		// lighting parameters
		r_Data.exposure = 0.5f;
		r_Data.gamma = 1.9f;
		r_Data.lightSize = 1.0f;
		r_Data.orthoSize = 20.0f;
		r_Data.lightNear = 20.0f;
		r_Data.lightFar = 200.0f;
		r_Data.intensity = 1.0f;

		//Point Lights initialization
		r_Data.numLights = 1024;
		r_Data.workGroupsX = (postProcFB.Width + (postProcFB.Width % 16)) / 16;
		r_Data.workGroupsY = (postProcFB.Height + (postProcFB.Height % 16)) / 16;
		//SetupLights();

		//Directional Light initialization
		r_Data.dirLightBuffer = UniformBuffer::Create(sizeof(r_Data.dirLight), 2);
		float dSize = r_Data.orthoSize;
		r_Data.lightProj = glm::ortho(-dSize, dSize, -dSize, dSize, r_Data.lightNear, r_Data.lightFar);
		r_Data.ShadowBuffer = UniformBuffer::Create(sizeof(glm::mat4), 3);
	}

	void ForwardPlusRenderer::Render()
	{
		auto view = r_Data.scene->m_Registry.view<TransformComponent, MeshComponent>();
		//-----------------------------------------------Depth Pre Pass--------------------------------------------//
		r_Data.depthPass->BindTargetFrameBuffer();
		RenderCommand::SetState(RenderState::DEPTH_TEST, true);
		RenderCommand::SetClearColor(r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
		r_Data.depthShader->Bind();
		RenderCommand::Clear();
		for (auto ent : view)
		{
			auto& tc = view.get<TransformComponent>(ent);
			auto& mc = view.get<MeshComponent>(ent);
			if (!mc.path.empty())
			{
				r_Data.depthShader->SetMat4("transform.u_trans", tc.GetTransform());
				Renderer::Submit(r_Data.depthShader, mc.model);
			}
		}
		r_Data.depthPass->UnbindTargetFrameBuffer();

		//----------------------------------------Directional Light Shadow Pass-----------------------------------//
		r_Data.shadowPass->BindTargetFrameBuffer();
		RenderCommand::SetState(RenderState::DEPTH_TEST, true);
		RenderCommand::SetClearColor(r_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
		r_Data.shadowDepthShader->Bind();
		RenderCommand::Clear();
		for (auto ent : view)
		{
			auto& tc = view.get<TransformComponent>(ent);
			auto& mc = view.get<MeshComponent>(ent);
			if (!mc.path.empty())
			{
				r_Data.shadowDepthShader->SetMat4("transform.u_trans", tc.GetTransform());
				Renderer::Submit(r_Data.shadowDepthShader, mc.model);
			}
		}
		r_Data.shadowPass->UnbindTargetFrameBuffer();
		
		//--------------------------------------------Light Culling-----------------------------------------------//
		//Compute pass to calculate light indices
		//r_Data.compShader->Bind();
		//Attaching depth map from previous pass
		//Texture2D::BindTexture(r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 0);
		// Define work group sizes in x and y direction based off screen size and tile size (in pixels)

		//TODO
		//Bind light buffers
		/*r_Data.compShader->DispatchCompute(r_Data.workGroupsX, r_Data.workGroupsY, 1);
		r_Data.compShader->Unbind();*/

		//---------------------------------------Lighting Accumulation--------------------------------------------//
		//r_Data.lightingPass->BindTargetFrameBuffer();

		//RenderCommand::SetState(RenderState::DEPTH_TEST, false);

		//r_Data.forwardLightingShader->Bind();
		////shadow map samplers
		//Texture2D::BindTexture(r_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 3);
		//Texture1D::BindTexture(r_Data.distributionSampler0->GetRendererID(), 4);
		//Texture1D::BindTexture(r_Data.distributionSampler1->GetRendererID(), 5);
		////Push constant variables
		////r_Data.deferredLighting->SetFloat("pc.size", r_Data.lightSize * 0.0001f);
		////r_Data.deferredLighting->SetInt("pc.numPCFSamples", r_Data.numPCF);
		////r_Data.deferredLighting->SetInt("pc.numBlockerSearchSamples", r_Data.numBlocker);
		////r_Data.deferredLighting->SetInt("pc.softShadow", (int)r_Data.softShadow);
		////r_Data.deferredLighting->SetFloat("pc.exposure", r_Data.exposure);
		////r_Data.deferredLighting->SetFloat("pc.gamma", r_Data.gamma);
		////r_Data.deferredLighting->SetFloat("pc.near", r_Data.lightNear);
		////r_Data.deferredLighting->SetFloat("pc.intensity", r_Data.intensity);

		//if (r_Data.environment) {
		//	r_Data.environment->SetIntensity(r_Data.intensity);
		//	r_Data.environment->BindIrradianceMap(7);
		//	r_Data.environment->BindPreFilterMap(8);
		//	r_Data.environment->BindBRDFMap(9);
		//}
		//Renderer::Submit(r_Data.forwardLightingShader, r_Data.screenVao);

		//r_Data.forwardLightingShader->Unbind();
		//r_Data.lightingPass->UnbindTargetFrameBuffer();
	}

	void ForwardPlusRenderer::End()
	{
		//r_Data.lightingPass->BindTargetFrameBuffer();
		//glBindFramebuffer(GL_READ_FRAMEBUFFER, r_Data.geoPass->GetSpecification().TargetFrameBuffer->GetRendererID());
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetRendererID()); // write to default framebuffer
		//auto w = r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width;
		//auto h = r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height;
		//glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		//if (r_Data.environment) {
		//	glEnable(GL_DEPTH_TEST);
		//	glDepthFunc(GL_LEQUAL);
		//	r_Data.environment->RenderBackground();
		//	glDepthFunc(GL_LESS);
		//}

		//-------------------------------------Post Processing and Depth Debug------------------------------------//
		r_Data.postProcPass->BindTargetFrameBuffer();
		r_Data.postProcShader->Bind();
		r_Data.screenVao->Bind();
		r_Data.postProcShader->SetInt("pc.useFXAA", r_Data.useFxaa==true ? 1:0);	
		r_Data.postProcShader->SetFloat("pc.width", (float)r_Data.postProcPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width);
		r_Data.postProcShader->SetFloat("pc.height", (float)r_Data.postProcPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height);
		//Texture2D::BindTexture(r_Data.lightingPass->GetFrameBufferTextureID(0), 0);
		Texture2D::BindTexture(r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 2);
		Renderer::Submit(r_Data.postProcShader, r_Data.screenVao);
		r_Data.postProcShader->Unbind();
		r_Data.postProcPass->UnbindTargetFrameBuffer();
		

	}

	void ForwardPlusRenderer::SetupLights()
	{
		glGenBuffers(1, &r_Data.lightBuffer);
		glGenBuffers(1, &r_Data.visibleLightIndicesBuffer);

		// Bind light buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_Data.lightBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, r_Data.numLights * sizeof(PointLight), 0, GL_DYNAMIC_DRAW);

		size_t numberOfTiles = r_Data.workGroupsX * r_Data.workGroupsY;

		// Bind visible light indices buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_Data.visibleLightIndicesBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numberOfTiles * sizeof(int) * 1024, 0, GL_STATIC_DRAW);

		//Setting point lights' values
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_Data.lightBuffer);
		auto pointLights = (pointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

		for (int i = 0; i < r_Data.numLights; i++) {
			pointLight& light = pointLights[i];
			light.color = glm::vec4(0);
			light.position = glm::vec4(0);
			light.radius = 10.0f;
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void ForwardPlusRenderer::UpdateLights()
	{
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
				r_Data.dirLight.color = glm::vec4(p->GetColor(), 0) * p->GetIntensity();
				r_Data.dirLight.direction = glm::vec4(p->GetDirection(), 0);
				r_Data.dirLight.position = glm::vec4(tc.Translation, 0);
				//shadow
				r_Data.lightView = glm::lookAt(-(glm::normalize(p->GetDirection()) * r_Data.lightFar / 4.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				r_Data.shadowData.lightViewProj = r_Data.lightProj * r_Data.lightView;
				r_Data.ShadowBuffer->SetData(&r_Data.shadowData, sizeof(glm::mat4));
				r_Data.dirLightBuffer->SetData(&r_Data.dirLight, sizeof(r_Data.dirLight));
				p = nullptr;
			}
			if (lc.type == LightType::Point) {
				if (pIndex < 1024) {
					//auto p = dynamic_cast<PointLight*>(lc.light.get());

					//glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_Data.lightBuffer);
					//auto pointLights = (pointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

					//pointLight& light = pointLights[pIndex];
					//light.color = glm::vec4(p->GetColor(), 1) * p->GetIntensity();
					//light.position = glm::vec4(tc.Translation, 1);
					//light.radius = p->GetRange();

					//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
					//glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

					//pIndex++;
					//p = nullptr;
				}
			}
			//if (lc.type == LightType::Spot) {
			//	if (sIndex < 4) {
			//		auto p = dynamic_cast<SpotLight*>(lc.light.get());
			//		r_Data.lightManager->UpdateSpotLights(p, tc.Translation, sIndex);
			//		sIndex++;
			//		p = nullptr;
			//	}
			//}
		}
	}

	uint32_t ForwardPlusRenderer::GetFinalTextureID(int index)
	{
		return r_Data.postProcPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(1);
	}

	Ref<FrameBuffer> ForwardPlusRenderer::GetMainFrameBuffer()
	{
		//TODO 
		//Fix attachment ID
		return r_Data.lightingPass->GetSpecification().TargetFrameBuffer;
	}

	void ForwardPlusRenderer::OnImGuiRender(bool* rendererOpen, bool* environmentOpen)
	{
		//TODO
		//Debugger for depth map and lights
		if (*rendererOpen) {
			ImGui::Begin(ICON_FA_COGS" Renderer Settings", rendererOpen);

			ImGui::Text("ForwardPlus debugger");
			static bool showDepth = false;
			if (ImGui::Button("Show Depth")) {
				showDepth = true;
			}
			auto width = r_Data.postProcPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width * 0.5f;
			auto height = r_Data.postProcPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height * 0.5f;
			auto ratio = height / width;
			width = ImGui::GetContentRegionAvail().x;
			height = ratio * width;
			if (showDepth) {
				ImGui::Begin("Show Depth", &showDepth);
				width = ImGui::GetContentRegionAvail().x;
				height = ratio * width;
				ImGui::Image((ImTextureID)r_Data.postProcPass->GetFrameBufferTextureID(1), { width, height }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
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

			//ImGui::Checkbox("Soft Shadow", &r_Data.softShadow);
			//ImGui::DragFloat("PCF samples", &r_Data.numPCF, 1, 1, 64);
			//ImGui::DragFloat("blocker samples", &r_Data.numBlocker, 1, 1, 64);


			//shadow
			//ImGui::DragFloat("Light Size", &r_Data.lightSize, 0.01f, 0, 100);

			//if (ImGui::DragFloat("Ortho Size", &r_Data.orthoSize, 0.1f, 1, 100)) {
			//	r_Data.lightProj = glm::ortho(-r_Data.orthoSize, r_Data.orthoSize, -r_Data.orthoSize, r_Data.orthoSize, r_Data.lightNear, r_Data.lightFar);
			//}

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
	}

	void ForwardPlusRenderer::OnResize(uint32_t width, uint32_t height)
	{
		r_Data.depthPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		r_Data.lightingPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		r_Data.postProcPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		//change the number of work groups in the compute shader
		auto w = r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width;
		auto h = r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height;
		r_Data.workGroupsX = (w + (w % 16)) / 16;
		r_Data.workGroupsY = (h + (h % 16)) / 16;
		size_t numberOfTiles = r_Data.workGroupsX * r_Data.workGroupsY;
		/*glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_Data.visibleLightIndicesBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numberOfTiles * sizeof(int) * 1024, 0, GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);*/
	}

}