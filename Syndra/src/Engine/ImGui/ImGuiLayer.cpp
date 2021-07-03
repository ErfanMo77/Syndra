#include "lpch.h"
#include "ImGuiLayer.h"
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "Engine/Core/Application.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include "IconsFontAwesome5.h"

namespace  Syndra {


	ImGuiLayer::ImGuiLayer()
		:Layer("ImGui Layer")
		,m_time(0)
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	void ImGuiLayer::OnAttach()
	{
		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;
		io.Fonts->AddFontFromFileTTF("assets/fonts/Montserrat-Black.ttf", 16.0f);
		io.Fonts->AddFontFromFileTTF("assets/fonts/Montserrat-Medium.ttf", 16.0f);
		io.Fonts->AddFontFromFileTTF("assets/fonts/Montserrat-Bold.ttf", 16.0f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/Montserrat-SemiBold.ttf", 16.0f);

		// merge in icons from Font Awesome
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config; 
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = 16.0f;
		//io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.ttf", 16.0f, &icons_config, icons_ranges);
		io.Fonts->AddFontFromFileTTF("assets/fonts/fa-regular-400.ttf", 16.0f, &icons_config, icons_ranges);
		io.Fonts->AddFontFromFileTTF("assets/fonts/fa-light-300.ttf", 16.0f, &icons_config, icons_ranges);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark(); 
		SetDarkThemeColors();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 450 core");
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnImGuiRender() 
	{


	}

	void ImGuiLayer::Begin() 
	{
		ImGui_ImplOpenGL3_NewFrame();
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
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
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