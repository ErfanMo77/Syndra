#include <Engine.h>
#include <iostream>

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
