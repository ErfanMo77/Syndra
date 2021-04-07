#include "lpch.h"
#include "EditorLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include <glad/glad.h>


namespace Syndra {

	EditorLayer::EditorLayer()
		:Layer("Editor Layer")
	{
		m_Camera = new PerspectiveCamera(45.0f, 1.66f, 0.1f, 1000.0f);
	}

	EditorLayer::~EditorLayer()
	{
		delete m_Camera;
	}

	void EditorLayer::OnAttach()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ScenePanel = CreateRef<ScenePanel>(m_ActiveScene);

		m_VertexArray = VertexArray::Create();
		m_QuadVA = VertexArray::Create();

		FramebufferSpecification fbSpec;
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		fbSpec.Samples = 4;
		m_OffScreenFB = FrameBuffer::Create(fbSpec);
		fbSpec.Samples = 1;
		m_PostprocFB = FrameBuffer::Create(fbSpec);


		m_ViewportSize = { fbSpec.Width,fbSpec.Height };

		float vertices[] = {
			// back face
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f,-1.0f,  // bottom-left
			 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f, 0.0f,-1.0f,// bottom-right    
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f,-1.0f, // top-right              
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f,-1.0f, // top-right
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 0.0f,-1.0f, // top-left
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f,-1.0f, // bottom-left                
			// front face
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f,1.0f, // bottom-left
			 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f,1.0f, // top-right
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 0.0f,1.0f, // bottom-right        
			 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f,1.0f, // top-right
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f,1.0f, // bottom-left
			-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 0.0f,1.0f, // top-left        
			// left face
			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  -1.0f, 0.0f,0.0f, // top-right
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  -1.0f, 0.0f,0.0f, // bottom-left
			-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  -1.0f, 0.0f,0.0f, // top-left       
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  -1.0f, 0.0f,0.0f, // bottom-left
			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  -1.0f, 0.0f,0.0f, // top-right
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  -1.0f, 0.0f,0.0f, // bottom-right
			// right face
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f, 0.0f,0.0f, // top-left
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  1.0f, 0.0f,0.0f, // top-right      
			 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f, 0.0f,0.0f, // bottom-right          
			 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f, 0.0f,0.0f, // bottom-right
			 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f, 0.0f,0.0f, // bottom-left
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f, 0.0f,0.0f, // top-left
			// bottom face          
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,0.0f, // top-right
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,0.0f, // bottom-left
			 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  0.0f, -1.0f,0.0f, // top-left        
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,0.0f, // bottom-left
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,0.0f, // top-right
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, -1.0f,0.0f, // bottom-right
			// top face
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 1.0f,0.0f, // top-left
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 1.0f,0.0f, // top-right
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 1.0f,0.0f, // bottom-right                 
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 1.0f,0.0f, // bottom-right
			-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 1.0f,0.0f, // bottom-left  
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 1.0f,0.0f  // top-left              
		};

		float quad[] = {
			// positions        // texture coords
			 1.0f,  1.0f, 0.0f,    1.0f, 1.0f,   // top right
			 1.0f, -1.0f, 0.0f,    1.0f, 0.0f,   // bottom right
			-1.0f, -1.0f, 0.0f,    0.0f, 0.0f,   // bottom left
			-1.0f,  1.0f, 0.0f,    0.0f, 1.0f    // top left 
		};


		m_VertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));
		m_QuadVB = VertexBuffer::Create(quad, sizeof(quad));

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float2,"a_uv"},
			{ShaderDataType::Float3,"a_normal"},
		};
		m_VertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(m_VertexBuffer);

		BufferLayout quadLayout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float2,"a_uv"},
		};
		m_QuadVB->SetLayout(quadLayout);
		m_QuadVA->AddVertexBuffer(m_QuadVB);
		
		unsigned int indices[] = { 0, 1, 2,
			2, 3, 0,
			// right
			1, 5, 6,
			6, 2, 1,
			// back
			7, 6, 5,
			5, 4, 7,
			// left
			4, 0, 3,
			3, 7, 4,
			// bottom
			4, 5, 1,
			1, 0, 4,
			// top
			3, 2, 6,
			6, 7, 3 };

		unsigned int quadIndices[] = {
			0, 1, 3, // first triangle
			1, 2, 3  // second triangle
		};
		m_IndexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		m_VertexArray->SetIndexBuffer(m_IndexBuffer);

		m_QuadIB = IndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t));
		m_QuadVA->SetIndexBuffer(m_QuadIB);

		m_Shaders.Load("assets/shaders/aa.glsl");
		auto postProcShader = m_Shaders.Get("aa");


		m_Shaders.Load("assets/shaders/diffuse.glsl");
		auto difShader = m_Shaders.Get("diffuse");
		difShader->Bind();

		m_Texture = Texture2D::Create("assets/Textures/tiles.jpg");
		m_Texture->Bind(0);

		
		difShader->SetInt("u_Texture", 0);
		RenderCommand::Init();

		auto& app = Application::Get();
		m_Camera->SetViewportSize((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());
		//Example
		m_CubeEntity = m_ActiveScene->CreateEntity("cube1");
		auto cubeEntity2 = m_ActiveScene->CreateEntity("cube2");
	}

	void EditorLayer::OnDetach()
	{
		
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		if (FramebufferSpecification spec = m_OffScreenFB->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != (uint32_t)m_ViewportSize.x || spec.Height != (uint32_t)m_ViewportSize.y))
		{	
			m_OffScreenFB->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_PostprocFB->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_Camera->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}
		if (Input::IsKeyPressed(Key::Escape)) {
			Application::Get().Close();
		}

		m_ActiveScene->OnUpdateEditor(ts, *m_Camera);

		m_OffScreenFB->Bind();
		//SN_INFO("Delta time : {0}ms", ts.GetMilliseconds());
		RenderCommand::SetClearColor(glm::vec4(m_ClearColor, 1.0f));
		RenderCommand::Clear();

		if (m_ViewportFocused) {
			m_Camera->OnUpdate(ts);
		}
		Renderer::BeginScene(*m_Camera);

		auto difShader = m_Shaders.Get("diffuse");
		glm::mat4 trans;
		trans = glm::scale(glm::mat4(1), m_Scale);
		difShader->Bind();
		m_Texture->Bind(0);
		difShader->SetMat4("u_trans", m_CubeEntity.GetComponent<TransformComponent>().GetTransform());
		difShader->SetFloat3("cameraPos", m_Camera->GetPosition());
		difShader->SetFloat3("lightPos", m_Camera->GetPosition());
		difShader->SetFloat3("cubeCol", m_CubeColor);

		Renderer::Submit(difShader, m_VertexArray);

		Renderer::EndScene();
		m_OffScreenFB->Unbind();

		auto postProcShader = m_Shaders.Get("aa");
		m_PostprocFB->Bind();
		glDisable(GL_DEPTH_TEST);
		//glDisable(GL_CULL_FACE);
		postProcShader->Bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_OffScreenFB->GetColorAttachmentRendererID());
		m_QuadVA->Bind();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		m_PostprocFB->Unbind();
		
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
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save")) {
					//TODO
				}
				if (ImGui::MenuItem("Exit"))
				{
					Application::Get().Close();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				//TODO : showing different tabs
			}
			ImGui::EndMenuBar();
		}

		m_ScenePanel->OnImGuiRender();

		ImGui::Begin("Scene");
		

		ImGui::ColorEdit3("cube color", glm::value_ptr(m_CubeColor));
		ImGui::ColorEdit3("clear color", glm::value_ptr(m_ClearColor));
		ImGui::End();

		//----------------------------------------------Renderer info-----------------------------------//
		ImGui::Begin("Renderer info");
		ImGui::Text(RenderCommand::GetInfo().c_str());
		ImGui::Text("\nApplication average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
		ImGui::Text("%d active windows (%d visible)", io.MetricsActiveWindows, io.MetricsRenderWindows);
		ImGui::Text("%d active allocations", io.MetricsActiveAllocations);
		ImGui::End();

		//----------------------------------------------Viewport----------------------------------------//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->SetBlockEvents(!m_ViewportFocused || !m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
		//m_Camera->SetViewportSize(viewportPanelSize.x, viewportPanelSize.y);
		//m_CameraController.OnResize(viewportPanelSize.x, viewportPanelSize.y);
		uint64_t textureID = m_PostprocFB->GetColorAttachmentRendererID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::End();


		//-----------------------------------------------Editor camera settings-------------------------------------//
		static bool camerSettings = true;
		if (camerSettings) {
			ImGui::Begin("Camera settings", &camerSettings);
			//Fov
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
			if (ImGui::SliderFloat("Near clip", &nearClip, 0.0001, 1)) {
				m_Camera->SetNearClip(nearClip);
			}
			ImGui::End();
		}
	}

	void EditorLayer::OnEvent(Event& event)
	{
		m_Camera->OnEvent(event);
	}

}
