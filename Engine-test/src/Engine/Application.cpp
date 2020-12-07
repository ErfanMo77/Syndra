#include "lpch.h"
#include "Application.h"


namespace Engine {

	Application::Application()
	{

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

		}
	}

}
