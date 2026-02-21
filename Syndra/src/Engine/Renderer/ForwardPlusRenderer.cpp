#include "lpch.h"
#include "ForwardPlusRenderer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Core/Application.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Core/Instrument.h"
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
			//main color
			FramebufferTextureFormat::RGBA8,
			//light debug
			FramebufferTextureFormat::RGBA8,
			//entities' ids for mouse click
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
		r_Data.orthoSize = 30.0f;
		r_Data.lightNear = 20.0f;
		r_Data.lightFar = 200.0f;
		r_Data.intensity = 1.0f;

		//Point Lights initialization
		r_Data.numLights = 256;
		r_Data.workGroupsX = (postProcFB.Width + (postProcFB.Width % 16)) / 16;
		r_Data.workGroupsY = (postProcFB.Height + (postProcFB.Height % 16)) / 16;
		SetupLights();

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
		{
			SN_PROFILE_SCOPE("Depth pass");
			r_Data.depthPass->BindTargetFrameBuffer();
			RenderCommand::SetState(RenderState::DEPTH_TEST, true);
			RenderCommand::SetClearColor(r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
			r_Data.depthShader->Bind();
			RenderCommand::Clear();
			for (auto ent : view)
			{
				auto& mc = view.get<MeshComponent>(ent);
				if (!mc.path.empty())
				{
					const glm::mat4 worldTransform = r_Data.scene->GetWorldTransform(Entity{ ent });
					r_Data.depthShader->SetMat4("transform.u_trans", worldTransform);
					Renderer::Submit(r_Data.depthShader, mc.model);
				}
			}
			r_Data.depthPass->UnbindTargetFrameBuffer();
		}
		//----------------------------------------Directional Light Shadow Pass-----------------------------------//
		{
			SN_PROFILE_SCOPE("Shadow Pass");
			r_Data.shadowPass->BindTargetFrameBuffer();
			RenderCommand::SetState(RenderState::DEPTH_TEST, true);
			RenderCommand::SetClearColor(r_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
			r_Data.shadowDepthShader->Bind();
			RenderCommand::Clear();
			for (auto ent : view)
			{
				auto& mc = view.get<MeshComponent>(ent);
				if (!mc.path.empty())
				{
					const glm::mat4 worldTransform = r_Data.scene->GetWorldTransform(Entity{ ent });
					r_Data.shadowDepthShader->SetMat4("transform.u_trans", worldTransform);
					Renderer::Submit(r_Data.shadowDepthShader, mc.model);
				}
			}
			r_Data.shadowPass->UnbindTargetFrameBuffer();
		}
		//--------------------------------------------Light Culling-----------------------------------------------//
		//Compute pass to calculate light indices
		{
			SN_PROFILE_SCOPE("Light Culling");
			r_Data.compShader->Bind();
			//Attaching depth map from previous pass
			Texture2D::BindTexture(r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 2);
			r_Data.compShader->SetMat4("pc.view", r_Data.scene->m_Camera->GetViewMatrix());
			r_Data.compShader->SetMat4("pc.projection", r_Data.scene->m_Camera->GetProjection());
			r_Data.compShader->SetFloat("pc.width", (float)r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width);
			r_Data.compShader->SetFloat("pc.height", (float)r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height);
			r_Data.compShader->SetInt("pc.lightCount", r_Data.numLights);
			//Define work group sizes in x and y direction based off screen size and tile size (in pixels)
			r_Data.compShader->DispatchCompute(r_Data.workGroupsX, r_Data.workGroupsY, 1);
			//glMemoryBarrier(GL_ALL_BARRIER_BITS);
			r_Data.compShader->Unbind();
		}
		//---------------------------------------Lighting Accumulation--------------------------------------------//
		{
			SN_PROFILE_SCOPE("Light Accumulation");
			r_Data.lightingPass->BindTargetFrameBuffer();
			RenderCommand::Clear();
			//RenderCommand::SetState(RenderState::DEPTH_TEST, true);
			r_Data.forwardLightingShader->Bind();
			r_Data.forwardLightingShader->SetInt("push.numberOfTilesX", r_Data.workGroupsX);
			//shadow map samplers
			Texture2D::BindTexture(r_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 5);
			Texture1D::BindTexture(r_Data.distributionSampler0->GetRendererID(), 6);
			Texture1D::BindTexture(r_Data.distributionSampler1->GetRendererID(), 10);
			//Push constant variables (lighting and shadow parameters)
			r_Data.forwardLightingShader->SetFloat("push.size", r_Data.lightSize * 0.0001f);
			r_Data.forwardLightingShader->SetInt("push.numPCFSamples", r_Data.numPCF);
			r_Data.forwardLightingShader->SetInt("push.numBlockerSearchSamples", r_Data.numBlocker);
			r_Data.forwardLightingShader->SetInt("push.softShadow", (int)r_Data.softShadow);
			r_Data.forwardLightingShader->SetFloat("push.exposure", r_Data.exposure);
			r_Data.forwardLightingShader->SetFloat("push.gamma", r_Data.gamma);
			r_Data.forwardLightingShader->SetFloat("push.near", r_Data.lightNear);
			r_Data.forwardLightingShader->SetFloat("push.intensity", r_Data.intensity);
			r_Data.forwardLightingShader->SetInt("push.disableShadow", r_Data.disableShadow);
			//Environment samplers
			if (r_Data.environment) {
				r_Data.environment->SetIntensity(r_Data.intensity);
				r_Data.environment->BindIrradianceMap(7);
				r_Data.environment->BindPreFilterMap(8);
				r_Data.environment->BindBRDFMap(9);
			}
			r_Data.forwardLightingShader->Bind();
			for (auto ent : view)
			{
				auto& mc = view.get<MeshComponent>(ent);
				if (!mc.path.empty())
				{
					const glm::mat4 worldTransform = r_Data.scene->GetWorldTransform(Entity{ ent });
					if (r_Data.scene->m_Registry.has<MaterialComponent>(ent)) {
						auto& mat = r_Data.scene->m_Registry.get<MaterialComponent>(ent);
						r_Data.forwardLightingShader->SetInt("transform.id", (uint32_t)ent);
						r_Data.forwardLightingShader->SetMat4("transform.u_trans", worldTransform);
						Renderer::Submit(mat.m_Material, mc.model);
					}
					else
					{
						r_Data.forwardLightingShader->SetInt("push.HasAlbedoMap", 1);
						r_Data.forwardLightingShader->SetFloat("push.tiling", 1.0f);
						r_Data.forwardLightingShader->SetInt("push.HasNormalMap", 0);
						r_Data.forwardLightingShader->SetInt("push.HasMetallicMap", 0);
						r_Data.forwardLightingShader->SetInt("push.HasRoughnessMap", 0);
						r_Data.forwardLightingShader->SetInt("push.HasAOMap", 0);
						r_Data.forwardLightingShader->SetFloat("push.material.MetallicFactor", 0);
						r_Data.forwardLightingShader->SetFloat("push.material.RoughnessFactor", 1);
						r_Data.forwardLightingShader->SetFloat("push.material.AO", 1);
						r_Data.forwardLightingShader->SetMat4("transform.u_trans", worldTransform);
						r_Data.forwardLightingShader->SetInt("transform.id", (uint32_t)ent);
						Renderer::Submit(r_Data.forwardLightingShader, mc.model);
					}
				}
			}
			r_Data.forwardLightingShader->Unbind();

			if (r_Data.environment) {
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LEQUAL);
				r_Data.environment->RenderBackground();
				glDepthFunc(GL_LESS);
			}
			r_Data.lightingPass->UnbindTargetFrameBuffer();
		}
	}

	void ForwardPlusRenderer::End()
	{
		//-------------------------------------Post Processing and Depth Debug------------------------------------//
		{
			SN_PROFILE_SCOPE("Post Processing");
			r_Data.postProcPass->BindTargetFrameBuffer();
			RenderCommand::Clear();
			r_Data.postProcShader->Bind();
			r_Data.screenVao->Bind();
			r_Data.postProcShader->SetInt("pc.useFXAA", r_Data.useFxaa == true ? 1 : 0);
			r_Data.postProcShader->SetFloat("pc.width", (float)r_Data.postProcPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width);
			r_Data.postProcShader->SetFloat("pc.height", (float)r_Data.postProcPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height);
			Texture2D::BindTexture(r_Data.lightingPass->GetFrameBufferTextureID(0), 0);
			Texture2D::BindTexture(r_Data.depthPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 2);
			Renderer::Submit(r_Data.postProcShader, r_Data.screenVao);
			r_Data.postProcShader->Unbind();
			r_Data.postProcPass->UnbindTargetFrameBuffer();
		}
	}

	void ForwardPlusRenderer::ShutDown()
	{
		glDeleteBuffers(1, &r_Data.lightBuffer);
		glDeleteBuffers(1, &r_Data.visibleLightIndicesBuffer);
	}

	void ForwardPlusRenderer::SetupLights()
	{
		glCreateBuffers(1, &r_Data.lightBuffer);
		glCreateBuffers(1, &r_Data.visibleLightIndicesBuffer);

		//creating and binding point light buffer
		glNamedBufferData(r_Data.lightBuffer, r_Data.numLights * sizeof(pointLight), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, r_Data.lightBuffer);

		size_t numberOfTiles = 121 * 56; //r_Data.workGroupsX * r_Data.workGroupsY;

		// Bind visible light indices buffer
		glNamedBufferData(r_Data.visibleLightIndicesBuffer, numberOfTiles * sizeof(VisibleIndex) * r_Data.numLights, nullptr, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, r_Data.visibleLightIndicesBuffer);

		//Setting point lights' values
		for (int i = 0; i < r_Data.numLights; i++) {
			auto& light = r_Data.pLights.lights[i];
			light.color = glm::vec4(0);
			light.position = glm::vec4(0);
			light.paddingAndRadius = glm::vec4(glm::vec3(0),00.0f);
		}
		glNamedBufferSubData(r_Data.lightBuffer, 0, sizeof(r_Data.pLights), &r_Data.pLights);
	}

	void ForwardPlusRenderer::UpdateLights()
	{
		SN_PROFILE_FUNCTION();
		auto viewLights = r_Data.scene->m_Registry.view<TransformComponent, LightComponent>();
		//point light index
		int pIndex = 0;
		//spot light index
		int sIndex = 0;
		//Set light values for each entity that has a light component

		//Setting point lights' values
		for (int i = 0; i < r_Data.numLights; i++) {
			auto& light = r_Data.pLights.lights[i];
			light.color = glm::vec4(0);
			light.position = glm::vec4(0);
			light.paddingAndRadius = glm::vec4(glm::vec3(0), 00.0f);
		}

		for (auto ent : viewLights)
		{
			auto& lc = viewLights.get<LightComponent>(ent);
			const glm::vec3 worldTranslation = r_Data.scene->GetWorldTranslation(Entity{ ent });

			if (lc.type == LightType::Directional) {
				auto p = dynamic_cast<DirectionalLight*>(lc.light.get());
				r_Data.dirLight.color = glm::vec4(p->GetColor(), 0) * p->GetIntensity();
				r_Data.dirLight.direction = glm::vec4(p->GetDirection(), 0);
				r_Data.dirLight.position = glm::vec4(worldTranslation, 0);
				//shadow
				r_Data.lightView = glm::lookAt(-(glm::normalize(p->GetDirection()) * r_Data.lightFar / 4.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				r_Data.shadowData.lightViewProj = r_Data.lightProj * r_Data.lightView;
				r_Data.ShadowBuffer->SetData(&r_Data.shadowData, sizeof(glm::mat4));
				r_Data.dirLightBuffer->SetData(&r_Data.dirLight, sizeof(r_Data.dirLight));
				p = nullptr;
			}
			if (lc.type == LightType::Point) {
				if (pIndex < 256) {
					auto p = dynamic_cast<PointLight*>(lc.light.get());

					pointLight& light = r_Data.pLights.lights[pIndex];
					light.color = glm::vec4(p->GetColor(), 1) * p->GetIntensity();
					light.position = glm::vec4(worldTranslation, 1);
					light.paddingAndRadius = glm::vec4(glm::vec3(0), p->GetRange());

					pIndex++;
					p = nullptr;
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
		//sending updated point lights to gpu
		glNamedBufferSubData(r_Data.lightBuffer, 0, sizeof(r_Data.pLights), &r_Data.pLights);
	}

	uint32_t ForwardPlusRenderer::GetFinalTextureID(int index)
	{
		return r_Data.postProcPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(0);
		//return r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(0);
	}

	uint32_t ForwardPlusRenderer::GetMouseTextureID()
	{
		return 2;
	}

	Ref<FrameBuffer> ForwardPlusRenderer::GetMainFrameBuffer()
	{
		//TODO
		//Fix attachment ID
		return r_Data.lightingPass->GetSpecification().TargetFrameBuffer;
	}

	void ForwardPlusRenderer::OnImGuiRender(bool* rendererOpen, bool* environmentOpen)
	{
		if (*rendererOpen) {
			ImGui::Begin(ICON_FA_COGS" Renderer Settings", rendererOpen);
			ImGui::Text("ForwardPlus debugger");
			static bool showDepth = false;
			static bool showlights = false;
			if (ImGui::Button("Show Depth")) {
				showDepth = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Show Lights")) {
				showlights = true;
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
				ImGui::Image(ImGuiLayer::GetTextureID(r_Data.postProcPass->GetFrameBufferTextureID(1)), { width, height }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				ImGui::End();
			}
			if (showlights) {
				ImGui::Begin("Show lights", &showlights);
				width = ImGui::GetContentRegionAvail().x;
				height = ratio * width;
				ImGui::Image(ImGuiLayer::GetTextureID(r_Data.lightingPass->GetFrameBufferTextureID(1)), { width, height }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
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

			ImGui::Checkbox("Disable Shadow", &r_Data.disableShadow);
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
				ImGui::Image(ImGuiLayer::GetTextureID(r_Data.environment->GetBackgroundTextureID()), { 300, 150 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			}
			if (ImGui::DragFloat("Intensity", &r_Data.intensity, 0.01f, 1, 20)) {
				if (r_Data.environment) {
					r_Data.environment->SetIntensity(r_Data.intensity);
				}
			}
			ImGui::End();
		}
	}

	void ForwardPlusRenderer::OnResize(uint32_t width, uint32_t height)
	{
		r_Data.depthPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		r_Data.lightingPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		r_Data.postProcPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		//change the number of work groups in the compute shader
		r_Data.workGroupsX = (width + (width % 16)) / 16;
		r_Data.workGroupsY = (height + (height % 16)) / 16;
		size_t numberOfTiles = r_Data.workGroupsX * r_Data.workGroupsY;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, r_Data.visibleLightIndicesBuffer);
		SN_CORE_INFO("Size:{0}", numberOfTiles * sizeof(VisibleIndex) * r_Data.numLights);
		SN_CORE_INFO("Width:{0} Height: {1}", width, height);
		glNamedBufferSubData(r_Data.visibleLightIndicesBuffer, 0, numberOfTiles * sizeof(VisibleIndex) * r_Data.numLights, nullptr);
	}

}
