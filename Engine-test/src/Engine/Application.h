#pragma once
#include "Core.h"
#include "Engine/Window.h"
#include "Engine/Events/ApplicationEvent.h"

namespace Engine {

	class ENGINE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
	private:
		std::unique_ptr<Window> m_window;
	};

	Application* CreateApplication();
}


