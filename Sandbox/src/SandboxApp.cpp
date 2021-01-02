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

	}

	virtual void OnImGuiRender() override
	{
		ImGui::ShowStyleEditor();
	}

	virtual void OnEvent(Syndra::Event& event) override {
		m_Camera->OnEvent(event);
	}


	virtual void OnAttach() override
	{
		m_VertexArray = Syndra::VertexArray::Create();

		float vertices[] = {
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
			 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
			 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
		};

		m_VertexBuffer = Syndra::VertexBuffer::Create(vertices, sizeof(vertices));

		Syndra::BufferLayout layout = {
			{Syndra::ShaderDataType::Float3,"a_pos"},
			{Syndra::ShaderDataType::Float2,"a_uv"},
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

		std::string vertexSrc = R"(
			#version 460 core
			
			layout(location = 0) in vec3 a_pos;
			layout(location = 1) in vec2 a_uv;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_trans;
				
			out vec3 v_pos;
			out vec2 v_uv;
			void main(){
				v_pos = a_pos;
				v_uv = a_uv;
				gl_Position = u_ViewProjection * u_trans *vec4(a_pos,1.0);
			}
			
		)";

		std::string fragSrc = R"(
			#version 460 core
			layout(location = 0) out vec4 color;	
			
			in vec3 v_pos;
			in vec2 v_uv;

			void main(){
				color = vec4(v_uv.x,0,0,1);
			}		
		)";
		m_Shader = Syndra::Shader::Create("test", vertexSrc, fragSrc);

		Syndra::RenderCommand::Init();
		m_Camera = new Syndra::PerspectiveCamera(45.0f,1.66f,0.1f,100.0f);
		m_Camera->SetViewportSize(1600, 900);
		
	}

	virtual void OnUpdate(Syndra::Timestep ts) override
	{
		SN_INFO("Delta time : {0}ms", ts.GetMilliseconds());
		Syndra::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		Syndra::RenderCommand::Clear();
		m_Camera->OnUpdate(ts);
		Syndra::Renderer::BeginScene(*m_Camera);

		m_Shader->SetMat4("u_trans", glm::translate(glm::mat4(1),glm::vec3(0)));
		Syndra::Renderer::Submit(m_Shader,m_VertexArray);

		Syndra::Renderer::EndScene();
	}


private:
	Syndra::Ref<Syndra::VertexArray> m_VertexArray;
	Syndra::Ref<Syndra::VertexBuffer> m_VertexBuffer;
	Syndra::Ref<Syndra::IndexBuffer> m_IndexBuffer;
	Syndra::Ref<Syndra::Shader> m_Shader;
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
