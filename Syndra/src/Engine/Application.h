#pragma once
#include "Core.h"
#include "Engine/Window.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"

namespace Engine {

	class ENGINE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void OnEvent(Event& e);
		void Run();

	private:
		
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		Scope<Window> m_window;
		bool m_Running;
	};

	Application* CreateApplication();
}


