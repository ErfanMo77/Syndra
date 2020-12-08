#include "lpch.h"
#include "Application.h"
#include "GLFW/glfw3.h"

namespace Engine {

	Application::Application()
	{
		m_window = std::unique_ptr<Window>(Window::Create());
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		std::cout << "Welcome to Engine!!!";
		WindowResizeEvent e(1280, 720);
		EG_TRACE(e);
		while (true)
		{
			glClearColor(0, 0.5, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			
			m_window->OnUpdate();
		}
	}

}
