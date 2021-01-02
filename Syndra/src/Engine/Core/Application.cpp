#include "lpch.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Input.h"
#include "GLFW/glfw3.h"


namespace Syndra {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		s_Instance = this;
		m_window = Scope<Window>(Window::Create());
		m_window->SetEventCallback(SN_BIND_EVENT_FN(Application::OnEvent));
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

	}

	Application::~Application()
	{

	}

	void Application::OnEvent(Event& e)
	{
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
		while (m_Running)
		{
			float time = (float)glfwGetTime();
			Timestep ts = time - m_lastFrameTime;
			m_lastFrameTime = time;

			for (Layer* layer : m_LayerStack) {
				layer->OnUpdate(ts);
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
