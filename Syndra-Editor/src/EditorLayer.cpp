#include "lpch.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "EditorLayer.h"
#include <glad/glad.h>
#include "imgui.h"
#include "ImGuizmo.h"

#include "Engine/Utils/Math.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Utils/PlatformUtils.h"



namespace Syndra {

	EditorLayer::EditorLayer()
		:Layer("Editor Layer")
	{
		m_Camera = new PerspectiveCamera(45.0f, 1.66f, 0.1f, 1000.0f);
		m_Info = RenderCommand::GetInfo();
	}

	EditorLayer::~EditorLayer()
	{
		delete m_Camera;
	}

	void EditorLayer::OnAttach()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ScenePanel = CreateRef<ScenePanel>(m_ActiveScene);
		//m_VertexArray = VertexArray::Create();
		//m_QuadVA = VertexArray::Create();

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8 , FramebufferTextureFormat::DEPTH24STENCIL8 };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		fbSpec.Samples = 4;
		//m_OffScreenFB = FrameBuffer::Create(fbSpec);
		//fbSpec.Samples = 1;
		//m_PostprocFB = FrameBuffer::Create(fbSpec);

		//fbSpec.Attachments = { FramebufferTextureFormat::RED_INTEGER , FramebufferTextureFormat::DEPTH24STENCIL8 };
		//m_MousePickFB = FrameBuffer::Create(fbSpec);

		m_ViewportSize = { fbSpec.Width,fbSpec.Height };

		//BufferLayout layout = {
		//	{ShaderDataType::Float3,"a_pos"},
		//	{ShaderDataType::Float2,"a_uv"},
		//	{ShaderDataType::Float3,"a_normal"},
		//};

		//float vertices[] = {
		//	// back face
		//	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f,-1.0f, // bottom-left
		//	 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f, 0.0f,-1.0f,// bottom-right    
		//	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f,-1.0f,// top-right              
		//	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f,-1.0f,// top-right
		//	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 0.0f,-1.0f,// top-left
		//	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f,-1.0f,// bottom-left                
		//	// front face
		//	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f,1.0f,  // bottom-left
		//	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f,1.0f,  // top-right
		//	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 0.0f,1.0f,  // bottom-right        
		//	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f,1.0f,  // top-right
		//	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f,1.0f,  // bottom-left
		//	-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 0.0f,1.0f,  // top-left        
		//	// left face
		//	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  -1.0f, 0.0f,0.0f, // top-right
		//	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  -1.0f, 0.0f,0.0f, // bottom-left
		//	-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  -1.0f, 0.0f,0.0f, // top-left       
		//	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  -1.0f, 0.0f,0.0f, // bottom-left
		//	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  -1.0f, 0.0f,0.0f, // top-right
		//	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  -1.0f, 0.0f,0.0f, // bottom-right
		//	// right face
		//	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f, 0.0f,0.0f,  // top-left
		//	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  1.0f, 0.0f,0.0f,  // top-right      
		//	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f, 0.0f,0.0f,  // bottom-right          
		//	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f, 0.0f,0.0f,  // bottom-right
		//	 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f, 0.0f,0.0f,  // bottom-left
		//	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f, 0.0f,0.0f,  // top-left
		//	// bottom face          
		//	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,0.0f, // top-right
		//	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,0.0f, // bottom-left
		//	 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  0.0f, -1.0f,0.0f, // top-left        
		//	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,0.0f, // bottom-left
		//	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,0.0f, // top-right
		//	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, -1.0f,0.0f, // bottom-right
		//	// top face										     
		//	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 1.0f,0.0f,  // top-left
		//	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 1.0f,0.0f,  // top-right
		//	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 1.0f,0.0f,  // bottom-right                 
		//	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 1.0f,0.0f,  // bottom-right
		//	-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 1.0f,0.0f,  // bottom-left  
		//	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 1.0f,0.0f  // top-left              
		//};

		//float quad[] = {
		//	// positions        // texture coords
		//	 1.0f,  1.0f, 0.0f,    1.0f, 1.0f,   // top right
		//	 1.0f, -1.0f, 0.0f,    1.0f, 0.0f,   // bottom right
		//	-1.0f, -1.0f, 0.0f,    0.0f, 0.0f,   // bottom left
		//	-1.0f,  1.0f, 0.0f,    0.0f, 1.0f    // top left 
		//};


		//m_VertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));
		//m_QuadVB = VertexBuffer::Create(quad, sizeof(quad));


		//m_VertexBuffer->SetLayout(layout);
		//m_VertexArray->AddVertexBuffer(m_VertexBuffer);

		//BufferLayout quadLayout = {
		//	{ShaderDataType::Float3,"a_pos"},
		//	{ShaderDataType::Float2,"a_uv"},
		//};
		//m_QuadVB->SetLayout(quadLayout);
		//m_QuadVA->AddVertexBuffer(m_QuadVB);
		//
		//unsigned int indices[] = { 0, 1, 2,
		//	2, 3, 0,
		//	// right
		//	1, 5, 6,
		//	6, 2, 1,
		//	// back
		//	7, 6, 5,
		//	5, 4, 7,
		//	// left
		//	4, 0, 3,
		//	3, 7, 4,
		//	// bottom
		//	4, 5, 1,
		//	1, 0, 4,
		//	// top
		//	3, 2, 6,
		//	6, 7, 3 };

		//unsigned int quadIndices[] = {
		//	0, 1, 3, // first triangle
		//	1, 2, 3  // second triangle
		//};
		//m_IndexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		//m_VertexArray->SetIndexBuffer(m_IndexBuffer);

		//m_QuadIB = IndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t));
		//m_QuadVA->SetIndexBuffer(m_QuadIB);

		//m_Shaders.Load("assets/shaders/aa.glsl");
		//m_Shaders.Load("assets/shaders/diffuse.glsl");
		//m_Shaders.Load("assets/shaders/mouse.glsl");
		//m_Shaders.Load("assets/shaders/outline.glsl");


		/*m_Texture = Texture2D::Create("assets/Textures/tiles.jpg");*/
		RenderCommand::Init();

		auto& app = Application::Get();
		m_Camera->SetViewportSize((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());
		////Example
		//m_CubeEntity = m_ActiveScene->CreateEntity("cube1");
		//m_CubeEntity2 = m_ActiveScene->CreateEntity("cube2");
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
			m_Camera->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		if (Input::IsKeyPressed(Key::Escape)) {
			Application::Get().Close();
		}

		m_ActiveScene->OnUpdateEditor(ts, *m_Camera);

		if (m_ViewportFocused && m_ViewportHovered) {
			m_Camera->OnUpdate(ts);
		}

		//m_OffScreenFB->Bind();

		//RenderCommand::SetClearColor(glm::vec4(m_ClearColor, 1.0f));
		//RenderCommand::Clear();
		//glEnable(GL_DEPTH_TEST);
		//Renderer::BeginScene(*m_Camera);
		//glm::mat4 trans;
		//trans = glm::scale(glm::mat4(1), m_Scale);
		//
		//auto difShader = m_Shaders.Get("diffuse");
		//difShader->Bind();
		//
		//m_Texture->Bind(0);
		//difShader->SetMat4("u_trans", glm::mat4(1.0));
		//difShader->SetFloat3("cameraPos", m_Camera->GetPosition());
		//difShader->SetFloat3("lightPos", m_Camera->GetPosition());
		////difShader->SetFloat3("cubeCol", m_CubeColor);
		//Renderer::Submit(difShader, *m_Model);
		//if (m_ScenePanel->GetSelectedEntity()) 
		//{
		//	auto outline = m_Shaders.Get("outline");
		//	outline->Bind();
		//	auto transform = m_ScenePanel->GetSelectedEntity().GetComponent<TransformComponent>();
		//	transform.Scale += glm::vec3(.05f);
		//	outline->SetMat4("u_trans", transform.GetTransform());
		//	Renderer::Submit(outline, m_VertexArray);
		//}
		////glDisable(GL_DEPTH_TEST);
		////Renderer::Submit(difShader, m_VertexArray);
		//Renderer::EndScene();


		//m_MousePickFB->Bind();
		//RenderCommand::SetClearColor(glm::vec4(m_ClearColor, 1.0f));
		//RenderCommand::Clear();
		//m_MousePickFB->ClearAttachment(0, -1);
		//glEnable(GL_DEPTH_TEST);
		//auto mouseShader = m_Shaders.Get("mouse");
		//mouseShader->Bind();
		//mouseShader->SetMat4("u_trans", m_CubeEntity2.GetComponent<TransformComponent>().GetTransform());
		//mouseShader->SetInt("u_ID", (uint32_t)m_CubeEntity2);
		//Renderer::Submit(mouseShader, m_VertexArray);

		//auto postProcShader = m_Shaders.Get("aa");

		//m_PostprocFB->Bind();
		//RenderCommand::Clear();
		//m_QuadVA->Bind();
		//
		//glDisable(GL_DEPTH_TEST);
		//postProcShader->Bind();
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_OffScreenFB->GetColorAttachmentRendererID());

		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		//m_PostprocFB->Unbind();

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

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
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

		m_ScenePanel->OnImGuiRender();

		ImGui::Begin("Scene settings");
		ImGui::ColorEdit3("cube color", glm::value_ptr(m_CubeColor));
		ImGui::ColorEdit3("clear color", glm::value_ptr(m_ClearColor));
		if (ImGui::Button("Reload shader")) {
			m_ActiveScene->ReloadShader();
		}
		

		ImGui::End();

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
		ImGui::Begin("Viewport");
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
		const glm::mat4& cameraProjection = m_Camera->GetProjection();
		glm::mat4 cameraView = m_Camera->GetViewMatrix();

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
		if (camerSettings) {
			ImGui::Begin("Camera settings", &camerSettings);
			//FOV
			float fov = m_Camera->GetFOV();
			if (ImGui::SliderFloat("Fov", &fov, 10, 180)) {
				m_Camera->SetFov(fov);
			}
			ImGui::Separator();
			//near-far clip
			float nearClip = m_Camera->GetNear();
			float farClip = m_Camera->GetFar();
			if (ImGui::SliderFloat("Far clip", &farClip , 10, 10000)) {
				m_Camera->SetFarClip(farClip);
			}
			ImGui::Separator();
			if (ImGui::SliderFloat("Near clip", &nearClip, 0.0001f, 1)) {
				m_Camera->SetNearClip(nearClip);
			}
			ImGui::End();
		}
	}

	void EditorLayer::OnEvent(Event& event)
	{
		m_Camera->OnEvent(event);
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
				m_Camera->SetFocalPoint(m_ScenePanel->GetSelectedEntity().GetComponent<TransformComponent>().Translation);
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
