#include "lpch.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "EditorLayer.h"
#include "imgui.h"
#include "ImGuizmo.h"

#include "Engine/Utils/Math.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/ImGui/IconsFontAwesome5.h"


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
		SceneRenderer::SetScene(m_ActiveScene);

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8 , FramebufferTextureFormat::DEPTH24STENCIL8 };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		fbSpec.Samples =1;

		m_ViewportSize = { fbSpec.Width,fbSpec.Height };
		RenderCommand::Init();
		OnLoadEditor();

		auto& app = Application::Get();
		m_ActiveScene->m_Camera->SetViewportSize((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());
		m_FullScreenIcon = Texture2D::Create("assets/Icons/fullscreen.png",false);
		m_GizmosIcon = Texture2D::Create("assets/Icons/globe.png",false);
		m_TransformIcon = Texture2D::Create("assets/Icons/Transform.png",false);
		m_RotationIcon = Texture2D::Create("assets/Icons/Rotation.png",false);
		m_ScaleIcon = Texture2D::Create("assets/Icons/scale.png",false);
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

		if (m_ViewportFocused && m_ViewportHovered) {
			m_ActiveScene->OnCameraUpdate(ts);
		}

		m_ActiveScene->OnUpdateEditor(ts);

	}

	void EditorLayer::OnImGuiRender()
	{
		//Dock Space
		UI::BeginDockSpace();

		//--------------------------------------------------MENU BARS-----------------------------------------//
		ShowMenuBars();

		//--------------------------------------------------Scene Panel---------------------------------------//
		if (!m_FullScreen) {
			m_ScenePanel->OnImGuiRender(&m_SceneHierarchyOpen, &m_PropertiesOpen);
		}

		//---------------------------------------------Editor's Camera settings--------------------------- ---//
		if (!m_FullScreen) {
			if (m_CameraSettingOpen) {
				ShowCameraSettings();
			}
		}
		//---------------------------------------------Renderer Settings--------------------------------------//
		if (!m_FullScreen) {
			SceneRenderer::OnImGuiRender(&m_RendererOpen, &m_EnvironmentOpen);
		}
	
		//---------------------------------------------Renderer Info------------------------------------------//	
		if (m_InfoOpen) {
			ShowRendererInfo();
		}

		//----------------------------------------------Viewport----------------------------------------------//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin(ICON_FA_IMAGE" Viewport");
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const ImGuiID id = window->GetID("Viewport");

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();

		//Gizmos Icons
		ShowIcons();

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		Application::Get().GetImGuiLayer()->SetBlockEvents(!m_ViewportFocused && !m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
		uint64_t textureID = m_ActiveScene->GetMainTextureID();
		ImGui::Image((ImTextureID)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		
		//Gizmos
		ShowGizmos();

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		if (m_ViewportFocused && m_ViewportHovered) {
			m_ActiveScene->m_Camera->OnEvent(event);
		}
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

		//Duplicate entity
		case Key::D:
		{
			if (control) {
				m_ScenePanel->CreateDuplicate();
			}
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
		case Key::T:
			m_GizmosChanged = true;
			m_GizmoMode == 0 ? m_GizmoMode = 1 : m_GizmoMode = 0;
			break;
		case Key::F:
		{
			if (m_ScenePanel->GetSelectedEntity()) {
				m_ActiveScene->m_Camera->SetFocalPoint(m_ScenePanel->GetSelectedEntity().GetComponent<TransformComponent>().Translation);
			}
			break;
		}
		case Key::Escape:
		{
			/*Application::Get().Close();*/
			m_ScenePanel->SetSelectedEntity({});
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
		if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y -35.0f && !altIsDown && m_ViewportHovered && !ImGuizmo::IsOver())
		{
			auto& mousePickFB = m_ActiveScene->GetMainFrameBuffer();
			mousePickFB->Bind();
			int pixelData = mousePickFB->ReadPixel(4, mouseX, mouseY);
			if (pixelData != -1) {
				m_ScenePanel->SetSelectedEntity(m_ActiveScene->FindEntity(pixelData));
			}
			else
			{
				if (!m_GizmosChanged) {
					m_ScenePanel->SetSelectedEntity({});
					m_GizmosChanged = true;
				}
			}
			//SN_CORE_WARN("pixel data: {0}", pixelData);
			mousePickFB->Unbind();
			
		}
		return false;
	}

	void EditorLayer::ShowMenuBars()
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem(ICON_FA_PLUS"  New", "Ctrl+N")) {
					NewScene();
				}
				ImGui::Separator();
				if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open...", "Ctrl+O")) {
					OpenScene();
				}
				ImGui::Separator();
				if (ImGui::MenuItem(ICON_FA_SAVE "  Save As...", "Ctrl+Shift+S")) {
					SaveSceneAs();
				}
				ImGui::Separator();
				if (ImGui::MenuItem(ICON_FA_WINDOW_CLOSE"  Exit"))
				{
					Application::Get().Close();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				//TODO : showing different tabs
				if (ImGui::MenuItem(ICON_FA_LIST_UL "  Scene hierarchy"))
					m_SceneHierarchyOpen = true;
				if (ImGui::MenuItem(ICON_FA_SLIDERS_H "  Properties"))
					m_PropertiesOpen = true;
				if (ImGui::MenuItem(ICON_FA_INFO "  Renderer Info"))
					m_InfoOpen = true;
				if (ImGui::MenuItem(ICON_FA_TREE"  Environment"))
					m_EnvironmentOpen = true;
				if (ImGui::MenuItem(ICON_FA_COGS" Renderer settings"))
					m_RendererOpen = true;
				if (ImGui::MenuItem(ICON_FA_CAMERA" Camera settings"))
					m_CameraSettingOpen = true;
				if (ImGui::MenuItem(ICON_FA_SYNC"  Reset Layout"))
					ResetLayout();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Entity")) {
				//TODO : showing different tabs
				if (ImGui::MenuItem("Add Entity")) {
					m_ActiveScene->CreateEntity();
				}
				if (ImGui::MenuItem("Add Cube")) {
					m_ActiveScene->CreatePrimitive(PrimitiveType::Cube);
				}
				if (ImGui::MenuItem("Add Sphere")) {
					m_ActiveScene->CreatePrimitive(PrimitiveType::Sphere);
				}
				if (ImGui::MenuItem("Add Plane")) {
					m_ActiveScene->CreatePrimitive(PrimitiveType::Plane);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	void EditorLayer::ShowIcons()
	{
		ImGui::Dummy({ 0,3 });
		ImGui::PushMultiItemsWidths(5, ImGui::CalcItemWidth() + 5);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0,0 });
		ImGui::Indent(5);
		//Full screen button
		if (ImGui::ImageButton((ImTextureID)m_FullScreenIcon->GetRendererID(), { 35,35 })) {
			m_FullScreen = !m_FullScreen;
		}
		if (ImGui::IsItemHovered()) {
			UI::Tooltip("Full screen");
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		//Global or local gizmos button
		ImGui::PushID("gizmos Type\0");

		if (ImGui::ImageButton((ImTextureID)m_GizmosIcon->GetRendererID(), { 35,35 })) {
			m_GizmosChanged = true;
			m_GizmoMode == 0 ? m_GizmoMode = 1 : m_GizmoMode = 0;
		}
		if (ImGui::IsItemHovered()) {
			UI::Tooltip("Local - World gizmos");
		}
		ImGui::PopItemWidth();
		ImGui::Unindent();
		ImGui::PopID();

		//Gizmos Icons
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)m_TransformIcon->GetRendererID(), { 35,35 }, { 0, 1 }, { 1, 0 })) {
			m_GizmoType = 7;
		}
		if (ImGui::IsItemHovered()) {
			UI::Tooltip("Translate");
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)m_RotationIcon->GetRendererID(), { 35,35 }, { 0, 1 }, { 1, 0 })) {
			m_GizmoType = 120;
		}
		if (ImGui::IsItemHovered()) {
			UI::Tooltip("Rotation");
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)m_ScaleIcon->GetRendererID(), { 35,35 }, { 0, 1 }, { 1, 0 })) {
			m_GizmoType = 896;
		}
		if (ImGui::IsItemHovered()) {
			UI::Tooltip("Scale");
		}
		ImGui::PopItemWidth();
		ImGui::PopStyleVar();
	}

	void EditorLayer::ShowGizmos()
	{
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();

		ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);
		const glm::mat4& cameraProjection = m_ActiveScene->m_Camera->GetProjection();
		glm::mat4 cameraView = m_ActiveScene->m_Camera->GetViewMatrix();

		Entity selectedEntity = m_ScenePanel->GetSelectedEntity();
		if (selectedEntity && m_GizmoType != -1)
		{
			// Entity transform
			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.GetTransform();

			// Snapping
			bool snap = Input::IsKeyPressed(Key::LeftControl);
			float snapValue = 1.0f; // Snap to 0.5m for translation/scale
			// Snap to 45 degrees for rotation
			if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
				snapValue = 45.0f;

			float snapValues[3] = { snapValue, snapValue, snapValue };

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
				(ImGuizmo::OPERATION)m_GizmoType, (ImGuizmo::MODE)m_GizmoMode, glm::value_ptr(transform),
				nullptr, snap ? snapValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translation, rotation, scale;
				Math::DecomposeTransform(transform, translation, rotation, scale);

				glm::vec3 deltaRotation = rotation - tc.Rotation;
				tc.Translation = translation;
				tc.Rotation += deltaRotation;
				tc.Scale = scale;
			}
		}
	}

	void EditorLayer::ShowCameraSettings()
	{
		ImGui::Begin(ICON_FA_CAMERA" Camera settings", &m_CameraSettingOpen);
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

	void EditorLayer::ShowRendererInfo()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Begin(ICON_FA_INFO" Renderer info", &m_InfoOpen);
		ImGui::Text(m_Info.c_str());
		ImGui::Text("\nApplication average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
		ImGui::Text("%d active windows (%d visible)", io.MetricsActiveWindows, io.MetricsRenderWindows);
		ImGui::Text("%d active allocations", io.MetricsActiveAllocations);
		ImGui::End();
	}

	// Load the default syndra scene when starting the engine
	void EditorLayer::OnLoadEditor()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_ScenePanel->SetContext(m_ActiveScene);

		SceneSerializer serializer(m_ActiveScene);
#ifdef SN_DEBUG
		serializer.Deserialize("assets/Scenes/Default.syndra");
#else
		serializer.Deserialize("assets/Scenes/Default_R.syndra");
#endif // SN_DEBUG
		SceneRenderer::SetScene(m_ActiveScene);
		Application::Get().GetWindow().SetTitle("Syndra Editor " + m_ActiveScene->m_Name + " scene");
	}

	void EditorLayer::ResetLayout()
	{
		m_InfoOpen = true;
		m_ViewportOpen = true;
		m_CameraSettingOpen = false;
		m_SceneHierarchyOpen = true;
		m_PropertiesOpen = true;
		m_RendererOpen = true;
		m_EnvironmentOpen = true;
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_ScenePanel->SetContext(m_ActiveScene);
		SceneRenderer::SetScene(m_ActiveScene);
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

			SceneRenderer::SetScene(m_ActiveScene);
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
