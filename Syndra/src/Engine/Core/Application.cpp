#include "lpch.h"
#include "Application.h"
#include "Input.h"
#include "GLFW/glfw3.h"
#include "glad/glad.h"

namespace Syndra {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		s_Instance = this;
		m_window = Scope<Window>(Window::Create());
		m_window->SetEventCallback(SN_BIND_EVENT_FN(Application::OnEvent));
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

		m_VertexArray = VertexArray::Create();
		
		float vertices[3 * 7] = {
			-0.5f, -0.5f, 0.0f, 0.0f,0.8f,0.0f,1.0f,
			 0.5f, -0.5f, 0.0f, 0.8f,0.3f,0.0f,1.0f,
			 0.0f,  0.5f, 0.0f, 0.1f,0.0,0.8f,1.0f
		};

		m_VertexBuffer = VertexBuffer::Create(vertices,sizeof(vertices));

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float4,"a_col"},
		};
		m_VertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(m_VertexBuffer);

		unsigned int indices[3] = { 0,1,2 };
		m_IndexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
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
		m_Shader.reset(new Shader(vertexSrc, fragSrc));

	}

	Application::~Application()
	{

	}

	void Application::OnEvent(Event& e)
	{
		if (e.IsInCategory(EventCategoryApplication))
		{
			SN_CORE_WARN(e);
		}
		else
		{
			SN_CORE_INFO(e);
		}

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(SN_BIND_EVENT_FN(Application::OnWindowClose));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}

	}

	void Application::Run()
	{
		SN_CORE_WARN("Driver: {0}",glGetString(GL_VENDOR));
		SN_CORE_WARN("Renderer: {0}",glGetString(GL_RENDERER));		
		SN_CORE_WARN("Version: {0}",glGetString(GL_VERSION));
		while (m_Running)
		{
			glClearColor(0.0f, 0.5f, 0.7f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			m_Shader->Bind();
			m_VertexArray->Bind();
			glDrawElements(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);

			for (Layer* layer : m_LayerStack) {
				layer->OnUpdate();
			}
			m_ImGuiLayer->Begin();
			for (Layer* Layer : m_LayerStack)
			{
				Layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();

			m_window->OnUpdate();
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

}
