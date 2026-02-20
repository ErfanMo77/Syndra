#include "lpch.h"
#include "ImGuiLayer.h"

#include "GLFW/glfw3.h"
#include "glad/glad.h"

#include "Engine/Core/Application.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Utils/AssetPath.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanImGuiTextureRegistry.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_vulkan.h"
#include "ImGuizmo.h"

#include "IconsFontAwesome5.h"

namespace {

	ImFont* AddEditorFont(ImGuiIO& io, const char* relativePath, float fontSize, const ImFontConfig* config = nullptr, const ImWchar* glyphRanges = nullptr)
	{
		const std::string fontPath = Syndra::AssetPath::ResolveEditorAssetPath(relativePath);
		ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, config, glyphRanges);
		SN_CORE_ASSERT(font != nullptr, "Could not load ImGui font file!");
		return font;
	}

	void CheckVkResult(VkResult result)
	{
		if (result != VK_SUCCESS)
		{
			SN_CORE_ERROR("ImGui Vulkan backend error: {}", static_cast<int32_t>(result));
		}
	}

	ImTextureID ToImGuiTextureID(uint64_t value)
	{
		return (ImTextureID)(uintptr_t)value;
	}

	ImTextureID ToImGuiTextureID(VkDescriptorSet descriptorSet)
	{
		return (ImTextureID)(uintptr_t)descriptorSet;
	}

}

namespace Syndra {

	ImGuiLayer* ImGuiLayer::s_Instance = nullptr;

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGui Layer")
	{
		s_Instance = this;
	}

	ImGuiLayer::~ImGuiLayer()
	{
		if (s_Instance == this)
			s_Instance = nullptr;
	}

	void ImGuiLayer::OnAttach()
	{
		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
			io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
		AddEditorFont(io, "assets/fonts/Montserrat-Black.ttf", 16.0f);
		AddEditorFont(io, "assets/fonts/Montserrat-Medium.ttf", 16.0f);
		AddEditorFont(io, "assets/fonts/Montserrat-Bold.ttf", 16.0f);
		io.FontDefault = AddEditorFont(io, "assets/fonts/Montserrat-SemiBold.ttf", 16.0f);

		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = 16.0f;
		AddEditorFont(io, "assets/fonts/fa-regular-400.ttf", 16.0f, &icons_config, icons_ranges);
		AddEditorFont(io, "assets/fonts/fa-light-300.ttf", 16.0f, &icons_config, icons_ranges);

		ImGui::StyleColorsDark();
		SetDarkThemeColors();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			m_Backend = Backend::Vulkan;
			VulkanContext* context = VulkanContext::GetCurrent();
			SN_CORE_ASSERT(context != nullptr, "Vulkan context is required before ImGui initialization.");
			SN_CORE_INFO("ImGui Vulkan: context acquired.");

			ImGui_ImplGlfw_InitForVulkan(window, true);
			SN_CORE_INFO("ImGui Vulkan: GLFW backend initialized.");

			VkFormat colorAttachmentFormat = context->GetSwapchainImageFormat();
			VkPipelineRenderingCreateInfo renderingCreateInfo{};
			renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
			renderingCreateInfo.colorAttachmentCount = 1;
			renderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;

			ImGui_ImplVulkan_InitInfo initInfo{};
			initInfo.ApiVersion = VK_API_VERSION_1_4;
			initInfo.Instance = context->GetInstance();
			initInfo.PhysicalDevice = context->GetPhysicalDevice();
			initInfo.Device = context->GetDevice();
			initInfo.QueueFamily = context->GetGraphicsQueueFamily();
			initInfo.Queue = context->GetGraphicsQueue();
			initInfo.MinImageCount = std::max(2u, context->GetSwapchainMinImageCount());
			initInfo.ImageCount = std::max(initInfo.MinImageCount, context->GetSwapchainImageCount());
			initInfo.DescriptorPoolSize = 2048;
			initInfo.UseDynamicRendering = true;
			initInfo.PipelineInfoMain.Subpass = 0;
			initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = renderingCreateInfo;
			initInfo.PipelineInfoForViewports = initInfo.PipelineInfoMain;
			initInfo.CheckVkResultFn = CheckVkResult;
			SN_CORE_INFO(
				"ImGui Vulkan handles: instance={}, physicalDevice={}, device={}",
				(void*)initInfo.Instance,
				(void*)initInfo.PhysicalDevice,
				(void*)initInfo.Device);

			const bool initialized = ImGui_ImplVulkan_Init(&initInfo);
			SN_CORE_INFO("ImGui Vulkan: backend init result={}.", initialized ? "true" : "false");
			SN_CORE_ASSERT(initialized, "Failed to initialize ImGui Vulkan backend.");
			m_LastVulkanMinImageCount = initInfo.MinImageCount;
			SN_CORE_INFO("ImGui Vulkan: initialization complete.");

			context->SetOverlayRenderCallback([this](VkCommandBuffer commandBuffer, uint32_t imageIndex)
				{
					RenderVulkanDrawData(commandBuffer, imageIndex);
				});
		}
		else
		{
			m_Backend = Backend::OpenGL;
			SN_CORE_INFO("ImGui OpenGL: initializing GLFW backend.");
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			SN_CORE_INFO("ImGui OpenGL: initializing OpenGL3 backend.");
			ImGui_ImplOpenGL3_Init("#version 450 core");
			SN_CORE_INFO("ImGui OpenGL: initialization complete.");
		}
	}

	void ImGuiLayer::OnDetach()
	{
		if (m_Backend == Backend::Vulkan)
		{
			VulkanContext* context = VulkanContext::GetCurrent();
			if (context != nullptr)
			{
				vkDeviceWaitIdle(context->GetDevice());
				context->SetOverlayRenderCallback({});
			}

			RemoveAllVulkanTextureDescriptors();
			ImGui_ImplVulkan_Shutdown();
		}
		else if (m_Backend == Backend::OpenGL)
		{
			ImGui_ImplOpenGL3_Shutdown();
		}

		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		m_Backend = Backend::None;
	}

	void ImGuiLayer::OnImGuiRender()
	{
	}

	void ImGuiLayer::Begin()
	{
		if (m_Backend == Backend::Vulkan)
		{
			VulkanContext* context = VulkanContext::GetCurrent();
			if (context != nullptr)
			{
				const uint32_t minImageCount = std::max(2u, context->GetSwapchainMinImageCount());
				if (minImageCount != m_LastVulkanMinImageCount)
				{
					ImGui_ImplVulkan_SetMinImageCount(minImageCount);
					m_LastVulkanMinImageCount = minImageCount;
				}
			}

			RemoveStaleVulkanTextureDescriptors();
			ImGui_ImplVulkan_NewFrame();
		}
		else if (m_Backend == Backend::OpenGL)
		{
			ImGui_ImplOpenGL3_NewFrame();
		}

		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		ImGui::Render();

		if (m_Backend == Backend::OpenGL)
		{
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backupCurrentContext);
			}
		}
		else if (m_Backend == Backend::Vulkan)
		{
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}
	}

	ImTextureID ImGuiLayer::GetTextureID(uint32_t rendererID)
	{
		if (s_Instance == nullptr)
		{
			return ToImGuiTextureID(rendererID);
		}

		return s_Instance->ResolveTextureID(rendererID);
	}

	ImTextureID ImGuiLayer::ResolveTextureID(uint32_t rendererID)
	{
		if (rendererID == 0)
			return (ImTextureID)0;

		if (m_Backend != Backend::Vulkan)
		{
			return ToImGuiTextureID(rendererID);
		}

		auto descriptorIterator = m_VulkanTextureCache.find(rendererID);
		if (descriptorIterator != m_VulkanTextureCache.end())
		{
			return ToImGuiTextureID(descriptorIterator->second);
		}

		VulkanImGuiTextureInfo textureInfo{};
		if (!VulkanImGuiTextureRegistry::TryGetTextureInfo(rendererID, textureInfo))
		{
			if (m_MissingVulkanTextureWarnings.insert(rendererID).second)
			{
				SN_CORE_WARN("Missing Vulkan ImGui texture registration for renderer ID {}.", rendererID);
			}
			return (ImTextureID)0;
		}

		const VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(textureInfo.Sampler, textureInfo.ImageView, textureInfo.ImageLayout);
		m_VulkanTextureCache[rendererID] = descriptorSet;
		m_MissingVulkanTextureWarnings.erase(rendererID);
		return ToImGuiTextureID(descriptorSet);
	}

	void ImGuiLayer::RenderVulkanDrawData(VkCommandBuffer commandBuffer, uint32_t)
	{
		if (m_Backend != Backend::Vulkan)
			return;

		ImDrawData* drawData = ImGui::GetDrawData();
		if (drawData == nullptr || drawData->CmdListsCount == 0)
			return;

		ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
	}

	void ImGuiLayer::RemoveStaleVulkanTextureDescriptors()
	{
		for (auto iterator = m_VulkanTextureCache.begin(); iterator != m_VulkanTextureCache.end();)
		{
			if (!VulkanImGuiTextureRegistry::ContainsTexture(iterator->first))
			{
				ImGui_ImplVulkan_RemoveTexture(iterator->second);
				m_MissingVulkanTextureWarnings.erase(iterator->first);
				iterator = m_VulkanTextureCache.erase(iterator);
				continue;
			}

			++iterator;
		}
	}

	void ImGuiLayer::RemoveAllVulkanTextureDescriptors()
	{
		for (const auto& [rendererID, descriptorSet] : m_VulkanTextureCache)
		{
			ImGui_ImplVulkan_RemoveTexture(descriptorSet);
			m_MissingVulkanTextureWarnings.erase(rendererID);
		}

		m_VulkanTextureCache.clear();
	}

	void ImGuiLayer::SetDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;

		auto& style = ImGui::GetStyle();
		style.WindowRounding = { 5 };
		style.ChildRounding = { 5 };
		style.FrameRounding = { 6 };
		style.GrabRounding = { 4 };
		style.FramePadding = { 4, 5 };


		colors[ImGuiCol_WindowBg] = ImVec4{ 0.25f, 0.25f, 0.25f, 1.000f };
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.18f, 0.21f, 0.24f, 1.00f);

		//// Headers
		colors[ImGuiCol_Header] = ImVec4(0.41f, 0.40f, 0.56f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.43f, 0.21f, 0.48f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.10f, 0.30f, 1.00f);

		//// Buttons
		colors[ImGuiCol_Button] = ImVec4(0.35f, 0.43f, 0.44f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.34f, 0.55f, 0.53f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.34f, 0.55f, 0.53f, 1.00f);

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.43f, 0.21f, 0.48f, 1.00f);

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4(0.504f, 0.392f, 0.540f, 0.862f);
		colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.05f, 0.25f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.10f, 0.30f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.58f, 0.46f, 0.57f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.41f, 0.40f, 0.56f, 1.00f);

		colors[ImGuiCol_CheckMark] = ImVec4(0.34f, 0.55f, 0.53f, 1.00f);

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4(0.35f, 0.43f, 0.44f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.34f, 0.55f, 0.53f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		colors[ImGuiCol_NavHighlight] = ImVec4(0.68f, 0.26f, 0.98f, 0.66f);

		colors[ImGuiCol_SliderGrab] = ImVec4{ 0.464f, 0.464f, 0.464f, 1.000f };

		colors[ImGuiCol_DockingPreview] = ImVec4(0.68f, 0.26f, 0.98f, 0.66f);
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

}
