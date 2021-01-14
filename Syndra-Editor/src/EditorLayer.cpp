#include "lpch.h"
#include "EditorLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"

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
		m_VertexArray = VertexArray::Create();

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

		m_VertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float2,"a_uv"},
			{ShaderDataType::Float3,"a_normal"},
		};
		m_VertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(m_VertexBuffer);

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
		m_IndexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		m_VertexArray->SetIndexBuffer(m_IndexBuffer);

		m_Shaders.Load("assets/shaders/diffuse.glsl");

		m_Texture = Texture2D::Create("assets/Textures/tiles.jpg");
		m_Texture->Bind(0);

		auto difShader = m_Shaders.Get("diffuse");
		difShader->SetInt("u_Texture", 0);
		RenderCommand::Init();

		auto& app = Application::Get();
		m_Camera->SetViewportSize(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		if (Input::IsKeyPressed(Key::Escape)) {
			Application::Get().Close();
		}
		//SN_INFO("Delta time : {0}ms", ts.GetMilliseconds());
		RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		RenderCommand::Clear();
		m_Camera->OnUpdate(ts);
		Renderer::BeginScene(*m_Camera);

		auto difShader = m_Shaders.Get("diffuse");

		difShader->SetMat4("u_trans", glm::translate(glm::mat4(1), glm::vec3(0)));
		difShader->SetFloat3("cameraPos", m_Camera->GetPosition());
		difShader->SetFloat3("lightPos", m_Camera->GetPosition());

		Renderer::Submit(difShader, m_VertexArray);

		Renderer::EndScene();
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
				if (ImGui::MenuItem("Exit"))
				{
					Application::Get().Close();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Begin("test");
		ImGui::Text("simple text");
		ImGui::End();

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		m_Camera->OnEvent(event);
	}

}
