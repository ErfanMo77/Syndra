#include "lpch.h"

#include "Engine/Renderer/SceneRenderer.h"

#include "Engine/Core/Instrument.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"
#include "imgui.h"

#include <array>
#include <unordered_map>

namespace Syndra {

	static SceneRenderer::SceneData s_Data;

	namespace {

		Ref<Shader> FindFirstExistingShader(const std::initializer_list<const char*> names)
		{
			for (const char* name : names)
			{
				if (s_Data.shaders.Exists(name))
					return s_Data.shaders.Get(name);
			}

			return nullptr;
		}

		glm::mat4 ConvertOpenGLClipToVulkanClip(const glm::mat4& matrix)
		{
			// Vulkan uses [0, 1] depth clip-space and opposite Y compared to the current OpenGL-style camera projection.
			glm::mat4 clip = glm::mat4(1.0f);
			clip[1][1] = -1.0f;
			clip[2][2] = 0.5f;
			clip[3][2] = 0.5f;
			return clip * matrix;
		}

	}

	void SceneRenderer::Initialize()
	{
		if (!s_Data.scene)
			return;

		if (s_Data.renderPipeline)
			s_Data.renderPipeline->ShutDown();

		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
			s_Data.renderPipeline = CreateRef<VulkanDeferredRenderer>();
		else
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
			if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
			{
				// Keep legacy names available so existing scenes/material references do not break.
				s_Data.shaders.Load("GeometryPass", "assets/shaders/vulkan/GeometryPass.glsl");
				s_Data.shaders.Load("DeferredLighting", "assets/shaders/vulkan/DeferredLighting.glsl");
				s_Data.shaders.Load("depth", "assets/shaders/vulkan/depth.glsl");
				s_Data.shaders.Load("FXAA", "assets/shaders/vulkan/FXAA.glsl");
				s_Data.shaders.Add("main", s_Data.shaders.Get("DeferredLighting"));
				s_Data.shaders.Add("ForwardShading", s_Data.shaders.Get("GeometryPass"));
			}
			else
			{
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
		}
		s_Data.main = ResolveShader("main");
		if (s_Data.scene)
			s_Data.scene->m_Shaders = s_Data.shaders;
	}

	void SceneRenderer::InitializeEnvironment()
	{
		if (!s_Data.scene)
			return;

		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
		{
			// Vulkan deferred pipeline manages environment sampling internally.
			s_Data.environment = nullptr;
			return;
		}

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
		SN_PROFILE_SCOPE("SceneRenderer::BeginScene");
		if (s_Data.environment)
		{
			s_Data.environment->SetViewProjection(camera.GetViewMatrix(), camera.GetProjection());
		}

		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
			s_Data.CameraBuffer.ViewProjection = ConvertOpenGLClipToVulkanClip(s_Data.CameraBuffer.ViewProjection);
		s_Data.CameraBuffer.position = glm::vec4(camera.GetPosition(), 0);
		if (s_Data.CameraUniformBuffer)
			s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(CameraData));

		Renderer::BeginScene(camera);
	}

	void SceneRenderer::RenderScene()
	{
		SN_PROFILE_SCOPE("SceneRenderer::RenderScene");
		if (!s_Data.renderPipeline)
			return;

		s_Data.renderPipeline->UpdateLights();
		s_Data.renderPipeline->Render();

	}

	void SceneRenderer::EndScene()
	{
		SN_PROFILE_SCOPE("SceneRenderer::EndScene");
		if (s_Data.renderPipeline)
			s_Data.renderPipeline->End();
	}

	void SceneRenderer::ShutDown()
	{
		if (s_Data.renderPipeline)
		{
			s_Data.renderPipeline->ShutDown();
			s_Data.renderPipeline = nullptr;
		}

		s_Data.main = nullptr;
		s_Data.CameraUniformBuffer = nullptr;
		s_Data.environment = nullptr;
		s_Data.scene = nullptr;
		s_Data.shaders = ShaderLibrary{};
	}

	void SceneRenderer::Reload(const Ref<Shader>& shader)
	{
		shader->Reload();
	}

	void SceneRenderer::OnViewPortResize(uint32_t width, uint32_t height)
	{
		if (s_Data.renderPipeline)
			s_Data.renderPipeline->OnResize(width, height);
	}

	void SceneRenderer::OnImGuiRender(bool* rendererOpen, bool* environmentOpen)
	{
		if (s_Data.renderPipeline)
			s_Data.renderPipeline->OnImGuiRender(rendererOpen, environmentOpen);

		if (!rendererOpen || !*rendererOpen)
			return;

		ImGui::Begin(ICON_FA_COGS" Shader Tools", rendererOpen);
		std::string label = "shader";
		static Ref<Shader> selectedShader;
		if (selectedShader)
			label = selectedShader->GetName();

		if (ImGui::BeginCombo("##Shaders", label.c_str()))
		{
			for (auto& shader : s_Data.shaders.GetShaders())
			{
				if (ImGui::Selectable(shader.first.c_str(), true))
					selectedShader = shader.second;

				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload shader") && selectedShader) {
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
		if (!s_Data.renderPipeline)
			return 0;

		return s_Data.renderPipeline->GetMouseTextureID();
	}

	uint32_t SceneRenderer::GetTextureID(int index)
	{
		if (!s_Data.renderPipeline)
			return 0;

		return s_Data.renderPipeline->GetFinalTextureID(index);
	}

	Syndra::FramebufferSpecification SceneRenderer::GetMainFrameSpec()
	{
		if (!s_Data.renderPipeline)
			return {};

		return s_Data.renderPipeline->GetMainFrameBuffer()->GetSpecification();
	}

	Syndra::Ref<Syndra::FrameBuffer> SceneRenderer::GetMainFrameBuffer()
	{
		if (!s_Data.renderPipeline)
			return nullptr;

		return s_Data.renderPipeline->GetMainFrameBuffer();
	}

	Syndra::ShaderLibrary& SceneRenderer::GetShaderLibrary()
	{
		return s_Data.shaders;
	}

	Ref<Shader> SceneRenderer::ResolveShader(const std::string& shaderName)
	{
		if (s_Data.shaders.Exists(shaderName))
			return s_Data.shaders.Get(shaderName);

		static const std::unordered_map<std::string, std::array<const char*, 4>> shaderAliases = {
			{ "ForwardShading", { "ForwardShading", "GeometryPass", "main", "DeferredLighting" } },
			{ "GeometryPass", { "GeometryPass", "ForwardShading", "main", "DeferredLighting" } },
			{ "main", { "main", "DeferredLighting", "ForwardShading", "GeometryPass" } },
			{ "DeferredLighting", { "DeferredLighting", "main", "GeometryPass", "ForwardShading" } }
		};

		const auto aliasIterator = shaderAliases.find(shaderName);
		if (aliasIterator != shaderAliases.end())
		{
			for (const char* aliasName : aliasIterator->second)
			{
				if (s_Data.shaders.Exists(aliasName))
					return s_Data.shaders.Get(aliasName);
			}
		}

		Ref<Shader> fallback = FindFirstExistingShader({ "ForwardShading", "GeometryPass", "main", "DeferredLighting" });
		if (fallback)
			return fallback;

		auto shaders = s_Data.shaders.GetShaders();
		if (!shaders.empty())
			return shaders.begin()->second;

		return nullptr;
	}

	Ref<Shader> SceneRenderer::GetDefaultMaterialShader()
	{
		return ResolveShader("ForwardShading");
	}

	bool SceneRenderer::SupportsEnvironmentControls()
	{
		return true;
	}

}
