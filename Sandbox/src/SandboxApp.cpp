#include <Engine.h>
#include "Engine/Core/EntryPoint.h"
#include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"

class DummyLayer : public Syndra::Layer {

public:
	DummyLayer() :Layer("Dummy") 
	{
	}
	
	~DummyLayer() {
		delete m_Camera;
	}

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("test");
		ImGui::Text("simple text");
		ImGui::End();
	}

	virtual void OnEvent(Syndra::Event& event) override {
		m_Camera->OnEvent(event);
	}


	virtual void OnAttach() override
	{
		m_VertexArray = Syndra::VertexArray::Create();

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

		m_VertexBuffer = Syndra::VertexBuffer::Create(vertices, sizeof(vertices));

		Syndra::BufferLayout layout = {
			{Syndra::ShaderDataType::Float3,"a_pos"},
			{Syndra::ShaderDataType::Float2,"a_uv"},
			{Syndra::ShaderDataType::Float3,"a_normal"},
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
		m_IndexBuffer = Syndra::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		m_VertexArray->SetIndexBuffer(m_IndexBuffer);
		
		m_Shaders.Load("assets/shaders/diffuse.glsl");

		m_Texture = Syndra::Texture2D::Create("assets/Textures/tiles.jpg");
		m_Texture->Bind(0);

		auto difShader = m_Shaders.Get("diffuse");
		difShader->SetInt("u_Texture", 0);
		Syndra::RenderCommand::Init();
		m_Camera = new Syndra::PerspectiveCamera(45.0f,1.66f,0.1f,1000.0f);
		auto& app = Syndra::Application::Get();

		m_Camera->SetViewportSize(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());
		
	}

	virtual void OnUpdate(Syndra::Timestep ts) override
	{
		if (Syndra::Input::IsKeyPressed(Syndra::Key::Escape)) {
			Syndra::Application::Get().Close();
		}
		//SN_INFO("Delta time : {0}ms", ts.GetMilliseconds());
		Syndra::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		Syndra::RenderCommand::Clear();
		m_Camera->OnUpdate(ts);
		Syndra::Renderer::BeginScene(*m_Camera);
		
		auto difShader = m_Shaders.Get("diffuse");

		difShader->SetMat4("u_trans", glm::translate(glm::mat4(1),glm::vec3(0)));
		difShader->SetFloat3("cameraPos", m_Camera->GetPosition());
		difShader->SetFloat3("lightPos", m_Camera->GetPosition());
		
		Syndra::Renderer::Submit(difShader,m_VertexArray);

		Syndra::Renderer::EndScene();
	}


private:
	Syndra::ShaderLibrary m_Shaders;

	Syndra::Ref<Syndra::VertexArray> m_VertexArray;
	Syndra::Ref<Syndra::VertexBuffer> m_VertexBuffer;
	Syndra::Ref<Syndra::IndexBuffer> m_IndexBuffer;
	Syndra::Ref<Syndra::Texture2D> m_Texture;
	Syndra::PerspectiveCamera* m_Camera;
};


class Sandbox : public Syndra::Application {

public:

	Sandbox() {
		PushLayer(new DummyLayer());
	}

	~Sandbox() {

	}

};

Syndra::Application* Syndra::CreateApplication() {
	return new Sandbox();
}
