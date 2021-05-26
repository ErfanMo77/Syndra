#include "lpch.h"
#include "imgui.h"

#include "Engine/Core/Application.h"
#include "Engine/Renderer/SceneRenderer.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Utils/PoissonGenerator.h"
#include <glad/glad.h>

namespace Syndra {

	static SceneRenderer::SceneData s_Data;

	void GeneratePoissonDisk(Ref<Texture1D>& sampler, size_t numSamples) {
		PoissonGenerator::DefaultPRNG PRNG;
		size_t attempts = 0;
		auto points = PoissonGenerator::generatePoissonPoints(numSamples * 2, PRNG);
		while (points.size() < numSamples && ++attempts < 100)
			auto points = PoissonGenerator::generatePoissonPoints(numSamples * 2, PRNG);
		if (attempts == 100)
		{
			SN_CORE_ERROR("couldn't generate Poisson-disc distribution with {0} samples", numSamples);
			numSamples = points.size();
		}
		std::vector<float> data(numSamples * 2);
		for (auto i = 0, j = 0; i < numSamples; i++, j += 2)
		{
			auto& point = points[i];
			data[j] = point.x;
			data[j + 1] = point.y;
		}

		sampler = Texture1D::Create(numSamples, &data[0]);
	}

	void SceneRenderer::Initialize()
	{

		//------------------------------------------------Deferred Geometry Render Pass----------------------------------------//
		FramebufferSpecification GeoFbSpec;
		GeoFbSpec.Attachments =
		{ 
			FramebufferTextureFormat::RGBA16F,			// Position texture attachment
			FramebufferTextureFormat::RGBA16F,			// Normal texture attachment
			FramebufferTextureFormat::RGBA16F,			// Albedo-specular texture attachment
			FramebufferTextureFormat::DEPTH24STENCIL8	// default depth map
		};
		GeoFbSpec.Width = 1280;
		GeoFbSpec.Height = 720;
		GeoFbSpec.Samples = 1;
		GeoFbSpec.ClearColor = glm::vec4(0.196f, 0.196f, 0.196f, 1.0f);

		RenderPassSpecification GeoPassSpec;
		GeoPassSpec.TargetFrameBuffer = FrameBuffer::Create(GeoFbSpec);
		s_Data.geoPass = RenderPass::Create(GeoPassSpec);

		//-------------------------------------------Lighting and Post Processing Pass---------------------------//
		FramebufferSpecification postProcFB;
		postProcFB.Attachments = { FramebufferTextureFormat::RGBA8 , FramebufferTextureFormat::DEPTH24STENCIL8 };
		postProcFB.Width = 1280;
		postProcFB.Height = 720;
		postProcFB.Samples = 1;
		postProcFB.ClearColor = glm::vec4(0.196f, 0.196f, 0.196f, 1.0f);

		RenderPassSpecification finalPassSpec;
		finalPassSpec.TargetFrameBuffer = FrameBuffer::Create(postProcFB);
		s_Data.lightingPass = RenderPass::Create(finalPassSpec);
		
		//-----------------------------------------------Shadow Pass---------------------------------------------//
		FramebufferSpecification shadowSpec;
		shadowSpec.Attachments = { FramebufferTextureFormat::DEPTH32 };
		shadowSpec.Width = 2048;
		shadowSpec.Height = 2048;
		shadowSpec.Samples = 1;
		shadowSpec.ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};

		RenderPassSpecification shadowPassSpec;
		shadowPassSpec.TargetFrameBuffer = FrameBuffer::Create(shadowSpec);
		s_Data.shadowPass = RenderPass::Create(shadowPassSpec);
		
		//------------------------------------------------Shaders------------------------------------------------//
		if (!s_Data.aa) {
			s_Data.shaders.Load("assets/shaders/diffuse.glsl");
			s_Data.shaders.Load("assets/shaders/main.glsl");
			s_Data.shaders.Load("assets/shaders/DeferredLighting.glsl");
			s_Data.shaders.Load("assets/shaders/GeometryPass.glsl");
			//s_Data.shaders.Load("assets/shaders/mouse.glsl");
			//s_Data.shaders.Load("assets/shaders/outline.glsl");
		}
		s_Data.aa = Shader::Create("assets/shaders/aa.glsl");
		s_Data.depth = Shader::Create("assets/shaders/depth.glsl");
		s_Data.geoShader = s_Data.shaders.Get("GeometryPass");

		s_Data.diffuse = s_Data.shaders.Get("diffuse");
		s_Data.main = s_Data.shaders.Get("main");
		s_Data.deferredLighting = s_Data.shaders.Get("DeferredLighting");

		//s_Data.mouseShader = s_Data.shaders.Get("mouse");
		//s_Data.outline = s_Data.shaders.Get("outline");

		//----------------------------------------------SCREEN QUAD---------------------------------------------//
		s_Data.screenVao = VertexArray::Create();
		float quad[] = {
			 1.0f,  1.0f, 0.0f,    1.0f, 1.0f,   // top right
			 1.0f, -1.0f, 0.0f,    1.0f, 0.0f,   // bottom right
			-1.0f, -1.0f, 0.0f,    0.0f, 0.0f,   // bottom left
			-1.0f,  1.0f, 0.0f,    0.0f, 1.0f    // top left 
		};
		s_Data.screenVbo = VertexBuffer::Create(quad, sizeof(quad));
		BufferLayout quadLayout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float2,"a_uv"},
		};
		s_Data.screenVbo->SetLayout(quadLayout);
		s_Data.screenVao->AddVertexBuffer(s_Data.screenVbo);
		unsigned int quadIndices[] = {
			0, 3, 1, // first triangle
			1, 3, 2  // second triangle
		};
		s_Data.screenEbo = IndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t));
		s_Data.screenVao->SetIndexBuffer(s_Data.screenEbo);
		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);

		//SN_CORE_TRACE("SIZE OF POINT LIGHTS : {0}", sizeof(s_Data.pointLights));
		//SN_CORE_TRACE("SIZE OF SPOT LIGHTS : {0}", sizeof(s_Data.spotLights));
		//SN_CORE_TRACE("SIZE OF DIRECTIONAL LIGHT : {0}", sizeof(s_Data.dirLight));
		s_Data.exposure = 0.5f;
		s_Data.gamma = 1.9f;
		s_Data.lightSize = 0.002f;
		//Light uniform Buffer layout: -- point lights -- spotlights -- directional light--
		s_Data.LightsBuffer = UniformBuffer::Create(sizeof(s_Data.pointLights) + sizeof(s_Data.spotLights) + sizeof(s_Data.dirLight), 2);
		
		GeneratePoissonDisk(s_Data.distributionSampler0, 64);
		GeneratePoissonDisk(s_Data.distributionSampler1, 64);

		s_Data.diffuse->Bind();
		Texture1D::BindTexture(s_Data.distributionSampler0->GetRendererID(), 4);
		Texture1D::BindTexture(s_Data.distributionSampler1->GetRendererID(), 5);
		s_Data.diffuse->Unbind();


		float dSize = 10.0f;
		s_Data.lightProj = glm::ortho(-dSize, dSize, -dSize, dSize, 1.0f, 500.0f);
		s_Data.ShadowBuffer = UniformBuffer::Create(sizeof(glm::mat4), 3);
	}

	void SceneRenderer::BeginScene(const PerspectiveCamera& camera)
	{
		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraBuffer.position = glm::vec4(camera.GetPosition(), 0);
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(CameraData));

		for (auto& pointLight : s_Data.pointLights) 
		{
			pointLight.color = glm::vec4(0);
			pointLight.position = glm::vec4(0);
			pointLight.dist = 0.0f;
		}

		for (auto& spot : s_Data.spotLights)
		{
			spot.color = glm::vec4(0);
			spot.position = glm::vec4(0);
			spot.direction = glm::vec4(0);
			spot.innerCutOff = glm::cos(glm::radians(12.5f));
			spot.outerCutOff = glm::cos(glm::radians(15.0f));
		}

		s_Data.dirLight.color = glm::vec4(0);
		s_Data.dirLight.position = glm::vec4(0);
		s_Data.dirLight.direction = glm::vec4(0);

		Renderer::BeginScene(camera);
	}

	void SceneRenderer::UpdateLights(Scene& scene) 
	{
		auto viewLights = scene.m_Registry.view<TransformComponent, LightComponent>();
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
				s_Data.dirLight.color = glm::vec4(lc.light->GetColor(), 0) * p->GetIntensity();
				s_Data.dirLight.direction = glm::vec4(p->GetDirection(), 0);
				s_Data.dirLight.position = glm::vec4(tc.Translation, 0);
				//shadow
				s_Data.lightView = glm::lookAt(-(glm::vec3(s_Data.dirLight.direction) * 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				s_Data.shadowData.lightViewProj = s_Data.lightProj * s_Data.lightView;
				s_Data.ShadowBuffer->SetData(&s_Data.shadowData, sizeof(glm::mat4));
				p = nullptr;
			}
			if (lc.type == LightType::Point) {
				if (pIndex < 4) {
					auto p = dynamic_cast<PointLight*>(lc.light.get());
					s_Data.pointLights[pIndex].color = glm::vec4(p->GetColor(), 1) * p->GetIntensity();
					s_Data.pointLights[pIndex].position = glm::vec4(tc.Translation, 1);
					s_Data.pointLights[pIndex].dist = p->GetRange();
					pIndex++;
					p = nullptr;
				}
			}
			if (lc.type == LightType::Spot) {
				if (sIndex < 4) {
					auto p = dynamic_cast<SpotLight*>(lc.light.get());
					s_Data.spotLights[sIndex].color = glm::vec4(p->GetColor(), 1) * p->GetIntensity();
					s_Data.spotLights[sIndex].position = glm::vec4(tc.Translation, 1);
					s_Data.spotLights[sIndex].direction = glm::vec4(p->GetDirection(), 0);
					s_Data.spotLights[sIndex].innerCutOff = glm::cos(glm::radians(p->GetInnerCutOff()));
					s_Data.spotLights[sIndex].outerCutOff = glm::cos(glm::radians(p->GetOuterCutOff()));
					sIndex++;
					p = nullptr;
				}
			}
		}

		//Filling light buffer data with different light values
		s_Data.LightsBuffer->SetData(&s_Data.pointLights, sizeof(s_Data.pointLights));
		s_Data.LightsBuffer->SetData(&s_Data.spotLights, sizeof(s_Data.spotLights), sizeof(s_Data.pointLights));
		s_Data.LightsBuffer->SetData(&s_Data.dirLight, sizeof(s_Data.dirLight), sizeof(s_Data.pointLights) + sizeof(s_Data.spotLights));

	}

	void SceneRenderer::RenderScene(Scene& scene)
	{

		UpdateLights(scene);

		auto view = scene.m_Registry.view<TransformComponent, MeshComponent>();
		//---------------------------------------------------------SHADOW PASS------------------------------------------//
		s_Data.shadowPass->BindTargetFrameBuffer();
		RenderCommand::SetState(RenderState::DEPTH_TEST, true);
		RenderCommand::SetClearColor(s_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
		s_Data.depth->Bind();
		RenderCommand::Clear();
		for (auto ent : view)
		{
			auto& tc = view.get<TransformComponent>(ent);
			auto& mc = view.get<MeshComponent>(ent);
			if (!mc.path.empty()) 
			{
				s_Data.depth->SetMat4("transform.u_trans", tc.GetTransform());
				SceneRenderer::RenderEntityColor(ent, tc, mc, s_Data.depth);
			}
		}
		s_Data.shadowPass->UnbindTargetFrameBuffer();

		//--------------------------------------------------GEOMETRY PASS----------------------------------------------//
		s_Data.geoPass->BindTargetFrameBuffer();
		RenderCommand::SetState(RenderState::DEPTH_TEST, true);
		RenderCommand::SetClearColor(s_Data.geoPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
		s_Data.geoShader->Bind();
		RenderCommand::Clear();
		for (auto ent : view)
		{
			auto& tc = view.get<TransformComponent>(ent);
			auto& mc = view.get<MeshComponent>(ent);
			if (!mc.path.empty()) 
			{
				if (scene.m_Registry.has<MaterialComponent>(ent)) {
					auto& mat = scene.m_Registry.get<MaterialComponent>(ent);
					SceneRenderer::RenderEntityColor(ent, tc, mc, mat);
				}
				else
				{
					s_Data.geoShader->SetMat4("transform.u_trans", tc.GetTransform());
					SceneRenderer::RenderEntityColor(ent, tc, mc, s_Data.geoShader);
				}
			}
		}
		s_Data.geoShader->Unbind();
		s_Data.geoPass->UnbindTargetFrameBuffer();
	}

	void SceneRenderer::RenderEntityColor(const entt::entity& entity, TransformComponent& tc, MeshComponent& mc,const Ref<Shader>& shader)
	{
		//RenderCommand::SetState(RenderState::CULL, false);
		Renderer::Submit(shader, mc.model);
		//RenderCommand::SetState(RenderState::CULL, true);
	}

	void SceneRenderer::RenderEntityColor(const entt::entity& entity, TransformComponent& tc, MeshComponent& mc, MaterialComponent& mat)
	{
		Renderer::Submit(mat.material, mc.model);
	}

	void SceneRenderer::EndScene()
	{
		//-------------------------------------------------Lighting and post-processing pass---------------------------------------------------//

		s_Data.lightingPass->BindTargetFrameBuffer();
		s_Data.screenVao->Bind();
		RenderCommand::SetState(RenderState::DEPTH_TEST, false);

		s_Data.deferredLighting->Bind();
		//shadow map samplers
		Texture2D::BindTexture(s_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 3);
		Texture1D::BindTexture(s_Data.distributionSampler0->GetRendererID(), 4);
		Texture1D::BindTexture(s_Data.distributionSampler1->GetRendererID(), 5);

		//Push constant variables
		s_Data.deferredLighting->SetFloat("pc.size", s_Data.lightSize);
		s_Data.deferredLighting->SetInt("pc.numPCFSamples", s_Data.numPCF);
		s_Data.deferredLighting->SetInt("pc.numBlockerSearchSamples", s_Data.numBlocker);
		s_Data.deferredLighting->SetInt("pc.softShadow", (int)s_Data.softShadow);
		s_Data.deferredLighting->SetFloat("pc.exposure", s_Data.exposure);
		s_Data.deferredLighting->SetFloat("pc.gamma", s_Data.gamma);
		//GBuffer samplers
		Texture2D::BindTexture(s_Data.geoPass->GetFrameBufferTextureID(0), 0);
		Texture2D::BindTexture(s_Data.geoPass->GetFrameBufferTextureID(1), 1);
		Texture2D::BindTexture(s_Data.geoPass->GetFrameBufferTextureID(2), 2);

		Renderer::Submit(s_Data.deferredLighting, s_Data.screenVao);

		s_Data.deferredLighting->Unbind();
		s_Data.lightingPass->UnbindTargetFrameBuffer();
		
		Renderer::EndScene();
	}

	void SceneRenderer::Reload()
	{
		s_Data.diffuse->Reload();
	}

	void SceneRenderer::OnViewPortResize(uint32_t width, uint32_t height)
	{
		s_Data.geoPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		s_Data.lightingPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		//s_Data.mouseFB->Resize(width, height);
	}

	void SceneRenderer::OnImGuiUpdate()
	{
		ImGui::Begin("Renderer settings");

		ImGui::Text("Geometry pass debugger");
		static bool showAlbedo = false;
		static bool showNormal = false;
		static bool showPosition = false;
		if (ImGui::Button("Albedo")) {
			showAlbedo = !showAlbedo;
		}
		ImGui::SameLine();
		if (ImGui::Button("Normal")) {
			showNormal = !showNormal;
		}
		ImGui::SameLine();
		if (ImGui::Button("Position")) {
			showPosition = !showPosition;
		}
		auto width = s_Data.geoPass->GetSpecification().TargetFrameBuffer->GetSpecification().Width * 0.5f;
		auto height = s_Data.geoPass->GetSpecification().TargetFrameBuffer->GetSpecification().Height * 0.5f;
		ImVec2 frameSize = ImVec2{ width,height };
		if (showAlbedo) {
			ImGui::Begin("Albedo");
			ImGui::Image(reinterpret_cast<void*>(s_Data.geoPass->GetFrameBufferTextureID(2)), frameSize, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::End();
		}

		if (showNormal) {
			ImGui::Begin("Normal");
			ImGui::Image(reinterpret_cast<void*>(s_Data.geoPass->GetFrameBufferTextureID(1)), frameSize, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::End();
		}

		if (showPosition) {
			ImGui::Begin("Position");
			ImGui::Image(reinterpret_cast<void*>(s_Data.geoPass->GetFrameBufferTextureID(0)), frameSize, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::End();
		}

		ImGui::NewLine();
		//V-Sync
		static bool vSync = true;
		ImGui::Checkbox("V-Sync", &vSync);
		Application::Get().GetWindow().SetVSync(vSync);

		//Exposure
		ImGui::DragFloat("exposure", &s_Data.exposure, 0.01f, -2, 4);

		//Gamma
		ImGui::DragFloat("gamma", &s_Data.gamma, 0.01f, 0, 4);
		ImGui::DragFloat("Size", &s_Data.lightSize,0.0001,0,100);

		//shadow
		ImGui::Checkbox("Soft Shadow", &s_Data.softShadow);
		ImGui::DragFloat("PCF samples", &s_Data.numPCF,1,1,64);
		ImGui::DragFloat("blocker samples", &s_Data.numBlocker,1,1,64);

		ImGui::End();
	}

	uint32_t SceneRenderer::GetTextureID(int index)
	{
		return s_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(index);
	}

	Syndra::FramebufferSpecification SceneRenderer::GetMainFrameSpec()
	{
		return s_Data.geoPass->GetSpecification().TargetFrameBuffer->GetSpecification();
	}

	Syndra::ShaderLibrary& SceneRenderer::GetShaderLibrary()
	{
		return s_Data.shaders;
	}

}
