#include "lpch.h"
#include "Application.h"
#include "GLFW/glfw3.h"

namespace Engine {

	Application::Application()
	{
		m_window = Scope<Window>(Window::Create());
		m_window->SetEventCallback(SN_BIND_EVENT_FN(Application::OnEvent));
	}

	Application::~Application()
	{

	}

	void Application::OnEvent(Event& e)
	{
		EG_CORE_TRACE(e);
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(SN_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(SN_BIND_EVENT_FN(Application::OnWindowResize));
	}

	void Application::Run()
	{
		std::cout << "Welcome to Engine!!!";
		WindowResizeEvent e(1280, 720);
		EG_TRACE(e);
		while (m_Running)
		{
			glClearColor(0, 0.5, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			
			m_window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		return false;
	}

}
