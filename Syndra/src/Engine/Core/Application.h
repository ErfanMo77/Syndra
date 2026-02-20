#pragma once
#include "Engine/Core/Core.h"
#include "Engine/Core/Layer.h"
#include "Engine/Core/LayerStack.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Core/Window.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Renderer/Renderer.h"

namespace Syndra {

	class Application
	{
	public:
		Application(const std::string& name = "");
		virtual ~Application();
		void OnEvent(Event& e);
		void Run();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_window; }
		ImGuiLayer* GetImGuiLayer() const { return m_ImGuiLayer; }

		void Close();

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		Ref<Window> m_window;
		ImGuiLayer* m_ImGuiLayer;
		float m_lastFrameTime = 0.0f;
		bool m_Running = true;
		bool m_Minimized = false;
		static Application* s_Instance;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();
}


