#include <Engine.h>
#include "Engine/Core/EntryPoint.h"
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

	virtual void OnAttach() override
	{
		m_VertexArray = Syndra::VertexArray::Create();

		float vertices[3 * 7] = {
			-0.5f, -0.5f, 0.0f, 1.0f,0.5f,0.0f,1.0f,
			 0.5f, -0.5f, 0.0f, 0.8f,0.4f,0.4f,1.0f,
			 0.0f,  0.5f, 0.0f, 0.1f,0.0,0.8f,1.0f
		};

		m_VertexBuffer = Syndra::VertexBuffer::Create(vertices, sizeof(vertices));

		Syndra::BufferLayout layout = {
			{Syndra::ShaderDataType::Float3,"a_pos"},
			{Syndra::ShaderDataType::Float4,"a_col"},
		};
		m_VertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(m_VertexBuffer);

		unsigned int indices[3] = { 0,1,2 };
		m_IndexBuffer = Syndra::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		m_VertexArray->SetIndexBuffer(m_IndexBuffer);

		std::string vertexSrc = R"(
			#version 460 core
			
			layout(location = 0) in vec3 a_pos;
			layout(location = 1) in vec4 a_col;
				
			out vec3 v_pos;
			out vec4 v_col;
			void main(){
				v_pos = a_pos;
				v_col = a_col;
				gl_Position = vec4(a_pos,1.0);
			}
			
		)";

		std::string fragSrc = R"(
			#version 460 core
			layout(location = 0) out vec4 color;	
			
			in vec3 v_pos;
			in vec4 v_col;

			void main(){
				color = v_col;
			}		
		)";
		m_Shader.reset(new Syndra::Shader(vertexSrc, fragSrc));
		Syndra::RenderCommand::Init();
	}

	virtual void OnUpdate() override
	{
		Syndra::RenderCommand::SetClearColor({ 0.0f, 0.5f, 0.7f, 1.0f });
		Syndra::RenderCommand::Clear();

		Syndra::Renderer::BeginScene();

		m_Shader->Bind();
		Syndra::Renderer::Submit(m_VertexArray);

		Syndra::Renderer::EndScene();
	}




private:
	Syndra::Ref<Syndra::VertexArray> m_VertexArray;
	Syndra::Ref<Syndra::VertexBuffer> m_VertexBuffer;
	Syndra::Ref<Syndra::IndexBuffer> m_IndexBuffer;
	Syndra::Ref<Syndra::Shader> m_Shader;
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
