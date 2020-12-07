#include <Engine.h>
#include "Engine/EntryPoint.h"

class Sandbox : public Engine::Application {

public:

	Sandbox() {

	}

	~Sandbox() {

	}

};

Engine::Application* Engine::CreateApplication() {
	return new Sandbox();
}
