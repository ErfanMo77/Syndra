#pragma once
#include "Core.h"
#include "Layer.h"
#include "LayerStack.h"
#include "Engine/Debug/ImGuiLayer.h"
#include "Engine/Window.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"

namespace Syndra {

	class Application
	{
	public:
		Application();
		virtual ~Application();
		void OnEvent(Event& e);
		void Run();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_window; }

	private:
		
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		Scope<Window> m_window;
		bool m_Running;
		static Application* s_Instance;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();
}


