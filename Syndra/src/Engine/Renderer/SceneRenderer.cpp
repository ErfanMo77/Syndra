#include "lpch.h"

#include "Engine/Renderer/SceneRenderer.h"

#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Engine/Renderer/Renderer.h"
#include <glad/glad.h>

namespace Syndra {

	static SceneRenderer::SceneData s_Data;
	static bool IsVulkan()
	{
		return Renderer::GetAPI() == RendererAPI::API::Vulkan;
	}

void SceneRenderer::Initialize()
{
	//s_Data.renderPipeline = CreateRef<DeferredRenderer>();
	s_Data.renderPipeline = CreateRef<ForwardPlusRenderer>();

		//Initializing the pipeline
		s_Data.renderPipeline->Init(s_Data.scene, s_Data.shaders, s_Data.environment);

		//----------------------------------------------Uniform BUffers---------------------------------------------//
		//TODO Should be moved to a different class?
		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);

	}

	void SceneRenderer::InitializeShaders()
	{
		//------------------------------------------------Shaders-----------------------------------------------//
		//Loading all shaders
		if (!s_Data.main) {
			s_Data.shaders.Load("assets/shaders/computeShader.cs");
			s_Data.shaders.Load("assets/shaders/diffuse.glsl");
			s_Data.shaders.Load("assets/shaders/FXAA.glsl");
			s_Data.shaders.Load("assets/shaders/main.glsl");
			s_Data.shaders.Load("assets/shaders/DeferredLighting.glsl");
			s_Data.shaders.Load("assets/shaders/GeometryPass.glsl");
			s_Data.shaders.Load("assets/shaders/depth.glsl");
			s_Data.shaders.Load("assets/shaders/depthPass.glsl");
			s_Data.shaders.Load("assets/shaders/ForwardShading.glsl");
			s_Data.shaders.Load("assets/shaders/ForwardPostProc.glsl");
			//s_Data.shaders.Load("assets/shaders/mouse.glsl");
			//s_Data.shaders.Load("assets/shaders/outline.glsl");
		}
		s_Data.main = s_Data.shaders.Get("main");
		s_Data.scene->m_Shaders = s_Data.shaders;
	}

void SceneRenderer::InitializeEnvironment()
{
	if (IsVulkan())
		return;
	//Initializing the environment map
	auto path = s_Data.scene->m_EnvironmentPath;
		if (s_Data.environment) {
			s_Data.scene->m_EnvironmentPath = s_Data.environment->GetPath();
		}
		if (!path.empty()) {
			s_Data.environment = CreateRef<Environment>(Texture2D::CreateHDR(path, false, true));
		}
	}

	//Initializing camera, uniform buffers and environment map
	void SceneRenderer::BeginScene(const PerspectiveCamera& camera)
	{
		if (s_Data.environment)
		{
			s_Data.environment->SetViewProjection(camera.GetViewMatrix(), camera.GetProjection());
		}

		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraBuffer.position = glm::vec4(camera.GetPosition(), 0);
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(CameraData));

		Renderer::BeginScene(camera);
	}

	void SceneRenderer::RenderScene()
	{

		s_Data.renderPipeline->UpdateLights();
		s_Data.renderPipeline->Render();
		
	}

	void SceneRenderer::EndScene()
	{
		s_Data.renderPipeline->End();
	}

	void SceneRenderer::ShutDown()
	{
		s_Data.renderPipeline->ShutDown();
	}

	void SceneRenderer::Reload(const Ref<Shader>& shader)
	{
		shader->Reload();
	}

	void SceneRenderer::OnViewPortResize(uint32_t width, uint32_t height)
	{
		s_Data.renderPipeline->OnResize(width, height);
	}

	void SceneRenderer::OnImGuiRender(bool* rendererOpen, bool* environmentOpen)
	{			
		s_Data.renderPipeline->OnImGuiRender(rendererOpen, environmentOpen);

		ImGui::Begin(ICON_FA_COGS" Renderer Settings", rendererOpen);
		std::string label = "shader";
		static Ref<Shader> selectedShader;
		if (selectedShader) {
			label = selectedShader->GetName();
		}
		static int item_current_idx = 0;
		static int index = 0;
		if (ImGui::BeginCombo("##Shaders", label.c_str()))
		{
			for (auto& shader : s_Data.shaders.GetShaders())
			{
				//const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(shader.first.c_str(), true)) {
					selectedShader = shader.second;
				}

				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload shader")) {
			SceneRenderer::Reload(selectedShader);
		}
		ImGui::Separator();
		ImGui::End();
	}

	void SceneRenderer::SetScene(const Ref<Scene>& scene)
	{
		s_Data.scene = scene;
	}

	void SceneRenderer::SetEnvironment(const Ref<Environment>& env)
	{
		s_Data.environment = env;
	}

	uint32_t SceneRenderer::GetMouseTextureID()
	{
		return s_Data.renderPipeline->GetMouseTextureID();
	}

	uint32_t SceneRenderer::GetTextureID(int index)
	{
		return s_Data.renderPipeline->GetFinalTextureID(index);
	}

	Syndra::FramebufferSpecification SceneRenderer::GetMainFrameSpec()
	{
		return s_Data.renderPipeline->GetMainFrameBuffer()->GetSpecification();
	}

	Syndra::Ref<Syndra::FrameBuffer> SceneRenderer::GetMainFrameBuffer()
	{
		return s_Data.renderPipeline->GetMainFrameBuffer();
	}

	Syndra::ShaderLibrary& SceneRenderer::GetShaderLibrary()
	{
		return s_Data.shaders;
	}

}
