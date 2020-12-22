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
		while (m_Running)
		{
			glClearColor(0, 0.5, 0.7, 1);
			glClear(GL_COLOR_BUFFER_BIT);
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
