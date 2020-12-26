#pragma once
#include "Engine/Core/Layer.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/KeyEvent.h"




namespace Syndra {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void Begin();
		void End();
		
		
	private:
		float m_time;
	};

}