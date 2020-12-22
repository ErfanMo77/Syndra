#include <Engine.h>
#include "Engine/EntryPoint.h"
#include "imgui.h"

class DummyLayer : public Syndra::Layer {
public:
	DummyLayer() :Layer("Dummy") 
	{
	}
	
	~DummyLayer() {

	}

	virtual void OnImGuiRender() override
	{
		ImGui::ShowStyleEditor();
	}

};


class Sandbox : public Syndra::Application {

public:

	Sandbox() {
		//PushLayer(new DummyLayer());
	}

	~Sandbox() {

	}

};

Syndra::Application* Syndra::CreateApplication() {
	return new Sandbox();
}
