#pragma once
#include "Core.h"
#include "Layer.h"
#include "LayerStack.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Core/Window.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Shader.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

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

	private:
		unsigned int m_VertexArray;
		Scope<Window> m_window;
		Scope<VertexBuffer> m_VertexBuffer;
		Scope<IndexBuffer> m_IndexBuffer;
		Scope<Shader> m_Shader;
		ImGuiLayer* m_ImGuiLayer;
		
		bool m_Running;
		static Application* s_Instance;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();
}


