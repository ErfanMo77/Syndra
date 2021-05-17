#include "lpch.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "EditorLayer.h"
#include "imgui.h"
#include "ImGuizmo.h"

#include "Engine/Utils/Math.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Utils/PlatformUtils.h"



namespace Syndra {

	EditorLayer::EditorLayer()
		:Layer("Editor Layer")
	{
		
		m_Info = RenderCommand::GetInfo();
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ScenePanel = CreateRef<ScenePanel>(m_ActiveScene);

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8 , FramebufferTextureFormat::DEPTH24STENCIL8 };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		fbSpec.Samples = 4;

		m_ViewportSize = { fbSpec.Width,fbSpec.Height };
		RenderCommand::Init();

		auto& app = Application::Get();
		m_ActiveScene->m_Camera->SetViewportSize((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());
	}

	void EditorLayer::OnDetach()
	{
		
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		if (FramebufferSpecification spec = m_ActiveScene->GetSpec();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != (uint32_t)m_ViewportSize.x || spec.Height != (uint32_t)m_ViewportSize.y))
		{	
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		if (Input::IsKeyPressed(Key::Escape)) {
			Application::Get().Close();
		}
		if (m_ViewportFocused && m_ViewportHovered) {
			m_ActiveScene->OnCameraUpdate(ts);
		}

		m_ActiveScene->OnUpdateEditor(ts);


	}

	void EditorLayer::OnImGuiRender()
	{
		static bool open = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->GetWorkPos());
			ImGui::SetNextWindowSize(viewport->GetWorkSize());
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &open, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
		style.WindowMinSize.x = minWinSizeX;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N")) {
					NewScene();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Open...", "Ctrl+O")) {
					OpenScene();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
					SaveSceneAs();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
				{
					Application::Get().Close();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				//TODO : showing different tabs
				if (ImGui::MenuItem("Scene hierarchy")) {
					//TODO
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Entity")) {
				//TODO : showing different tabs
				if (ImGui::MenuItem("Add Entity")) {
					m_ActiveScene->CreateEntity();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (!m_FullScreen) {
			m_ScenePanel->OnImGuiRender();
		}
		//----------------------------------------------Scene Settings-----------------------------------//
		if (!m_FullScreen) {
			ImGui::Begin("Scene settings");
			ImGui::ColorEdit3("cube color", glm::value_ptr(m_CubeColor));
			ImGui::ColorEdit3("clear color", glm::value_ptr(m_ClearColor));
			if (ImGui::Button("Reload shader")) {
				m_ActiveScene->ReloadShader();
			}
			static bool vSync = true;
			ImGui::Checkbox("V-Sync", &vSync);
			Application::Get().GetWindow().SetVSync(vSync);

			static float exposure = 1.f;
			ImGui::DragFloat("exposure", &exposure, 0.01f, -2, 4);
			SceneRenderer::SetExposure(exposure);

			static float gamma = 1.f;
			ImGui::DragFloat("gamma", &gamma, 0.01f, 0, 4);
			SceneRenderer::SetGamma(gamma);

			ImGui::End();
		}
		


		//----------------------------------------------Renderer info-----------------------------------//
		ImGui::Begin("Renderer info");
		ImGui::Text(m_Info.c_str());
		ImGui::Text("\nApplication average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
		ImGui::Text("%d active windows (%d visible)", io.MetricsActiveWindows, io.MetricsRenderWindows);
		ImGui::Text("%d active allocations", io.MetricsActiveAllocations);
		ImGui::End();


		//----------------------------------------------Viewport----------------------------------------//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		static bool viewOpen = true;
		ImGui::Begin("Viewport", &viewOpen);
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const ImGuiID id = window->GetID("Viewport");

		ImGui::Dummy({ 0,3 });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0,5 });
		if (ImGui::ImageButton(io.Fonts->TexID, { 20,20 })) {
			m_FullScreen = !m_FullScreen;
		}
		ImGui::PopStyleVar();


		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();

		Application::Get().GetImGuiLayer()->SetBlockEvents(!m_ViewportFocused && !m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
		uint64_t textureID = m_ActiveScene->GetMainTextureID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();

		ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);
		const glm::mat4& cameraProjection = m_ActiveScene->m_Camera->GetProjection();
		glm::mat4 cameraView = m_ActiveScene->m_Camera->GetViewMatrix();

		/*ImGuizmo::DrawGrid(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), glm::value_ptr(glm::mat4(1.0f)), 10);*/
		
		// Gizmos
		Entity selectedEntity = m_ScenePanel->GetSelectedEntity();
		if (selectedEntity && m_GizmoType != -1)
		{
			// Entity transform
			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.GetTransform();

			// Snapping
			bool snap = Input::IsKeyPressed(Key::LeftControl);
			float snapValue = 0.5f; // Snap to 0.5m for translation/scale
			// Snap to 45 degrees for rotation
			if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
				snapValue = 45.0f;

			float snapValues[3] = { snapValue, snapValue, snapValue };

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
				(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
				nullptr, snap ? snapValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translation, rotation, scale;
				Math::DecomposeTransform(transform, translation,rotation, scale);

				glm::vec3 deltaRotation = rotation - tc.Rotation;
				tc.Translation = translation;
				tc.Rotation += deltaRotation;
				tc.Scale = scale;
			}
		}


		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::End();


		//-----------------------------------------------Editor camera settings-------------------------------------//
		static bool camerSettings = true;
		if (!m_FullScreen) {
			if (camerSettings) {
				ImGui::Begin("Camera settings", &camerSettings);
				//FOV
				float fov = m_ActiveScene->m_Camera->GetFOV();
				if (ImGui::SliderFloat("Fov", &fov, 10, 180)) {
					m_ActiveScene->m_Camera->SetFov(fov);
				}
				ImGui::Separator();
				//near-far clip
				float nearClip = m_ActiveScene->m_Camera->GetNear();
				float farClip = m_ActiveScene->m_Camera->GetFar();
				if (ImGui::SliderFloat("Far clip", &farClip, 10, 10000)) {
					m_ActiveScene->m_Camera->SetFarClip(farClip);
				}
				ImGui::Separator();
				if (ImGui::SliderFloat("Near clip", &nearClip, 0.0001f, 1)) {
					m_ActiveScene->m_Camera->SetNearClip(nearClip);
				}
				ImGui::End();
			}
		}
	}

	void EditorLayer::OnEvent(Event& event)
	{
		m_ActiveScene->m_Camera->OnEvent(event);
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>(SN_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(SN_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// Shortcuts
		if (e.GetRepeatCount() > 0)
			return false;

		bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
		
		switch (e.GetKeyCode())
		{
		case Key::N:
		{
			if (control)
				NewScene();

			break;
		}
		case Key::O:
		{
			if (control)
				OpenScene();

			break;
		}
		case Key::S:
		{
			if (control && shift)
				SaveSceneAs();

			break;
		}

		// Gizmos
		case Key::Q:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = -1;
			break;
		}
		case Key::W:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		}
		case Key::E:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		}
		case Key::R:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		}

		case Key::F:
		{
			if (m_ScenePanel->GetSelectedEntity()) {
				m_ActiveScene->m_Camera->SetFocalPoint(m_ScenePanel->GetSelectedEntity().GetComponent<TransformComponent>().Translation);
			}
			break;
		}
		default: break;
		}
		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		my = viewportSize.y - my;
		int mouseX = (int)mx;
		int mouseY = (int)my;
		altIsDown = Input::IsKeyPressed(Key::LeftAlt);
		if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y && !altIsDown && m_ViewportHovered && !ImGuizmo::IsOver())
		{
			auto& mousePickFB = m_ActiveScene->GetMouseFrameBuffer();
			mousePickFB->Bind();
			int pixelData = mousePickFB->ReadPixel(0, mouseX, mouseY);
			if (pixelData != -1) {
				m_ScenePanel->SetSelectedEntity(m_ActiveScene->FindEntity(pixelData));
			}
			else
			{
				m_ScenePanel->SetSelectedEntity({});
			}
			//SN_CORE_WARN("pixel data: {0}", pixelData);
			mousePickFB->Unbind();
		}
		return false;
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_ScenePanel->SetContext(m_ActiveScene);
	}

	void EditorLayer::OpenScene()
	{
		std::optional<std::string> filepath = FileDialogs::OpenFile("Syndra Scene (*.syndra)\0*.syndra\0");
		if (filepath)
		{
			m_ActiveScene = CreateRef<Scene>();
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_ScenePanel->SetContext(m_ActiveScene);
			
			SceneSerializer serializer(m_ActiveScene);
			serializer.Deserialize(*filepath);
			Application::Get().GetWindow().SetTitle("Syndra Editor "+m_ActiveScene->m_Name+ " scene");
		}
	}

	void EditorLayer::SaveSceneAs()
	{
		std::optional<std::string> filepath = FileDialogs::SaveFile("Syndra Scene (*.syndra)\0*.syndra\0");
		if (filepath)
		{
			SceneSerializer serializer(m_ActiveScene);
			serializer.Serialize(*filepath);
		}
	}

}
