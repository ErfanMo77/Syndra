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
		ImGui::ShowStyleEditor();
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

		std::string vertexSrc = R"(
			#version 460 core
			
			layout(location = 0) in vec3 a_pos;
			layout(location = 1) in vec2 a_uv;
			layout(location = 2) in vec3 a_normal;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_trans;

			out vec3 v_pos;
			out vec2 v_uv;
			out vec3 v_normal;
			void main(){
				v_normal = mat3(transpose(inverse(u_trans)))*a_normal;
				v_pos = vec3(u_trans*vec4(a_pos,1.0));
				v_uv = a_uv;
				gl_Position = u_ViewProjection * u_trans *vec4(a_pos,1.0);
			}
			
		)";

		std::string fragSrc = R"(
			#version 460 core
			layout(location = 0) out vec4 fragColor;	
			
			uniform sampler2D u_Texture;

			uniform vec3 cameraPos;
			uniform vec3 lightPos;

			in vec3 v_pos;
			in vec2 v_uv;
			in vec3 v_normal;
			void main(){	
			    vec3 norm = normalize(v_normal);
				vec3 lightDir = normalize(lightPos - v_pos);
				vec3 viewDir = normalize(cameraPos - v_pos);
				vec3 reflectDir = reflect(-lightDir,norm);

				float diff = max(dot(norm,lightDir),0);
				vec3 color = vec3(texture(u_Texture,v_uv));
				vec3 result = (diff+0.2)*color;
				
				fragColor = vec4(result,1.0);
			}		
		)";
		m_Shader = Syndra::Shader::Create("test", vertexSrc, fragSrc);

		m_Texture = Syndra::Texture2D::Create("Assests/Textures/tiles.jpg");
		float mem = (float)(m_Texture->GetWidth() * m_Texture->GetHeight()* 4 * 1.33f) / 1048576.0f;
		SN_INFO("memory allocated for textures:{0}",mem);
		m_Texture->Bind(0);
		m_Shader->SetInt("u_Texture", 0);
		Syndra::RenderCommand::Init();
		m_Camera = new Syndra::PerspectiveCamera(45.0f,1.66f,0.1f,1000.0f);
		auto& app = Syndra::Application::Get();

		m_Camera->SetViewportSize(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());
		
	}

	virtual void OnUpdate(Syndra::Timestep ts) override
	{
		//SN_INFO("Delta time : {0}ms", ts.GetMilliseconds());
		Syndra::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		Syndra::RenderCommand::Clear();
		m_Camera->OnUpdate(ts);
		Syndra::Renderer::BeginScene(*m_Camera);

		m_Shader->SetMat4("u_trans", glm::translate(glm::mat4(1),glm::vec3(0)));
		m_Shader->SetFloat3("cameraPos", m_Camera->GetPosition());
		m_Shader->SetFloat3("lightPos", m_Camera->GetPosition());
		
		Syndra::Renderer::Submit(m_Shader,m_VertexArray);

		Syndra::Renderer::EndScene();
	}


private:
	Syndra::Ref<Syndra::VertexArray> m_VertexArray;
	Syndra::Ref<Syndra::VertexBuffer> m_VertexBuffer;
	Syndra::Ref<Syndra::IndexBuffer> m_IndexBuffer;
	Syndra::Ref<Syndra::Shader> m_Shader;
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
