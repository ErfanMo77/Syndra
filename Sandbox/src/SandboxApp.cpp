#include <Engine.h>
#include "Engine/EntryPoint.h"

class Sandbox : public Syndra::Application {

public:

	Sandbox() {
		PushLayer(new Syndra::ImGuiLayer());
	}

	~Sandbox() {

	}

};

Syndra::Application* Syndra::CreateApplication() {
	return new Sandbox();
}
